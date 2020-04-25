<?php

class FindasticWidgetSlider extends FindasticWidget {

  protected $defaults = array(
    'user_input' => TRUE,
    'step' => 0.1,
    'positive_prices' => TRUE,
    'decimals' => 2,
    'decimals_separator' => ',',
    'thousands_separator' => '.',
  );

  /**
   * Builds the settings form for this widget.
   */
  function settingsForm(&$form, &$form_state) {
    parent::settingsForm($form, $form_state);

    $form['user_input'] = array(
      '#type' => 'checkbox',
      '#title' => t('Allow user input'),
      '#default_value' => $this->user_input,
    );

    $form['display_suffix'] = array(
      '#type' => 'textfield',
      '#title' => t('Suffix'),
      '#default_value' => $this->display_suffix,
      '#size' => 5,
    );

    $form['step'] = array(
      '#type' => 'textfield',
      '#title' => t('Slider Step'),
      '#default_value' => $this->step,
      '#size' => 5,
    );

    $form['positive_prices'] = array(
      '#type' => 'checkbox',
      '#title' => t('Hide zero priced products'),
      '#default_value' => $this->positive_prices,
      '#size' => 5,
    );

    $form['decimals'] = array(
      '#type' => 'textfield',
      '#title' => t('Decimals'),
      '#default_value' => $this->decimals,
      '#size' => 2,
    );

    $form['decimals_separator'] = array(
      '#type' => 'textfield',
      '#title' => t('Decimals separator'),
      '#default_value' => $this->decimals_separator,
      '#size' => 1,
    );

    $form['thousands_separator'] = array(
      '#type' => 'textfield',
      '#title' => t('Thousands separator'),
      '#default_value' => $this->thousands_separator,
      '#size' => 1,
    );
  }

  /**
   * Returns the current widget value.
   */
  public function value() {
    $values = array();

    // Add selected values if exist.
    if (isset($_GET['min-' . $this->field->name])) {
      $values['min'] = $_GET['min-' . $this->field->name];
    }

    if (isset($_GET['max-' . $this->field->name])) {
      $values['max'] = $_GET['max-' . $this->field->name];
    }

    return $values;
  }

  /**
   * Adds current value to a url.
   */
  function addFacetValueToURL($value = FALSE) {
    $name = $this->field->name;
    $values = $this->value;
    return @array_filter(array(
      "min-{$name}" => $value ? $value['min'] : $values['min'],
      "max-{$name}" => $value ? $value['max'] : $values['max'],
    ));
  }

  /**
   * Builds the facet query for this widget type.
   */
  public function facetQuery(&$payload) {
    $payload['params']['stats'] = "true";
    $payload['params']['stats.field'][] = "{!ex={$this->facetName}}{$this->facetName}";

    // Add min value.
    if (!empty($this->value['min'])) {
      $payload['params']['fq'][] = "{!tag={$this->facetName},filter}{$this->facetName}:[{$this->value['min']} TO *]";
    }

    // Add max value.
    if (!empty($this->value['max'])) {
      $payload['params']['fq'][] = "{!tag={$this->facetName},filter}{$this->facetName}:[* TO {$this->value['max']}]";
    }

    // Check positive prices for the current entity type.
    if ($this->positive_prices) {
      $payload['params']['fq'][] = "{!tag={$this->facetName}}({$this->facetName}:[0 TO *] OR entity_type:(* -{$this->entity_type}))";
    }
  }

  /**
   * Builds the facet query for this widget type.
   */
  public function textQuery(&$payload, &$words) {
    $from = array_filter(explode("\n", variable_get('findastic_keyword_from', "από\nαπο\nάνω\nανω\nπάνω\nπανω\n>\napo\nano\nanw\npano\npanw\nfrom")));
    $to = array_filter(explode("\n", variable_get('findastic_keyword_to', "μέχρι\nμεχρι\nέως\nεως\nως\nκάτω\nκατω\n<\nmexri\neos\news\nto\nkato\nkatw")));
    $equal = array_filter(explode("\n", variable_get('findastic_keyword_equal', "με\n=\nme")));
    $about = array_filter(explode("\n", variable_get('findastic_keyword_about', "περιπου\nπερίπου\n~\nperipou\nperipoy")));

    // Add queries depending the keywords from, to, equal and about.
    foreach($words as $i => $word) {
      // If "from" keyword is found.
      if (in_array($word, $from)) {
        $j = $i;
        do {
          $j++;
          $nextword = isset($words[$j]) ? str_replace(',', '.', str_replace('€', '', $words[$j])) : FALSE;
        }
        while ($nextword and !is_numeric($nextword));
        if ($nextword) {
          for ($k = $i; $k <= $j; $k++) {
            unset($words[$k]);
          }
          $payload['params']['fq'][] = $this->facetName . ':[' . ((float) $nextword) . ' TO *]';
        }
      }

      // If "to" keyword is found.
      if (in_array($word, $to)) {
        $j = $i;
        do {
          $j++;
          $nextword = isset($words[$j]) ? str_replace(',', '.', str_replace('€', '', $words[$j])) : FALSE;
        }
        while ($nextword and !is_numeric($nextword));
        if ($nextword) {
          for ($k = $i; $k <= $j; $k++) {
            unset($words[$k]);
          }
          $payload['params']['fq'][] = $this->facetName . ':[0.001 TO ' . ((float) $nextword) . ']';
        }
      }

      // if "about" keyword is found.
      if (in_array($word, $about)) {
        $j = $i;
        do {
          $j++;
          $nextword = isset($words[$j]) ? str_replace(',', '.', str_replace('€', '', $words[$j])) : FALSE;
        }
        while ($nextword and !is_numeric($nextword));
        if ($nextword) {
          for ($k = $i; $k <= $j; $k++) {
            unset($words[$k]);
          }
          $payload['params']['fq'][] = $this->facetName . ':[' . ((float) $nextword - 20) . ' TO ' . ((float) $nextword + 20) . ']';
        }
      }

      // If "equal" keyword is found.
      if (in_array($word, $equal) and (!isset($words[$i+1]) or !in_array($words[$i+1], $about))) {
        $j = $i;
        do {
          $j++;
          $nextword = isset($words[$j])?str_replace(',', '.', str_replace('€', '', $words[$j])):FALSE;
        }
        while ($nextword and !is_numeric($nextword));
        if ($nextword) {
          for ($k = $i; $k <= $j; $k++) {
            unset($words[$k]);
          }
          $payload['params']['fq'][] = $this->facetName . ':' . ((float) $nextword);
        }
      }
    }
  }

  /**
   * Prepares the result from solr search
   */
  public function prepareResult(&$result, $search) {
    $stats = $search['stats']['stats_fields'][$this->facetName];
    $stats['min'] = floor($stats['min']);
    $stats['max'] = ceil($stats['max']);
    $result->stats[$this->field->name] = $stats;
  }

  /**
   * Adds some json data to the response.
   */
  public function json($result) {
    $min = $result->stats[$this->field->name]['min'];
    $max = $result->stats[$this->field->name]['max'];
    $values = $this->value;
    $current_min = isset($values['min']) ? $values['min'] : $min;
    $current_max = isset($values['max']) ? $values['max'] : $max;
    return array(
      'min' => $min,
      'max' => $max,
      'values' => array($current_min, $current_max),
    );
  }

  /**
   * Renders a clear link.
   */
  public function clearLinks($result) {
    $items = array();
    $values = $this->value;

    if (!empty($values)) {
      $min = isset($values['min']) ? $values['min'] : $result->stats[$this->field->name]['min'];
      $max = isset($values['max']) ? $values['max'] : $result->stats[$this->field->name]['max'];
      $query = $this->facetURL(FALSE);
      $name = $this->field->name;
      unset($query['min-' . $name]);
      unset($query['max-' . $name]);

      $min = number_format($min, $this->decimals, $this->decimals_separator, $this->thousands_separator);
      $max = number_format($max, $this->decimals, $this->decimals_separator, $this->thousands_separator);
      $items[] =  theme('link', array('text' => t('@min to @max', array('@min' => $min . $this->display_suffix, '@max' => $max . $this->display_suffix)), 'path' => $result->path, 'options' => array(
        'query' => $query,
        'html' => TRUE,
        'attributes' => array(
          'rel' => 'nofollow',
        ),
      )));
    }

    return $items;
  }

  /**
   * Renders the widget element.
   */
  function render($result) {
    $widget = array();

    $min = $result->stats[$this->field->name]['min'];
    $max = $result->stats[$this->field->name]['max'];
    $values = $this->value;
    $current_min = isset($values['min']) ? $values['min'] : $min;
    $current_max = isset($values['max']) ? $values['max'] : $max;

    // Add slider library.
    $widget['#attached']['library'][] = array('findastic', 'findastic.slider');

    // Build slider.
    $widget['slider'] = array(
      '#type' => 'container',
      '#attributes' => array(
        'id' => 'findastic-slider-' . $this->id,
        'class' => array('findastic-slider'),
      ),
    );

    $widget['slider']['range'] = array(
      '#type' => 'container',
      '#attributes' => array(
        'class' => 'findastic-slider-range',
        'data-slider-id' => $this->id,
        'data-slider-step' => $this->step,
        'data-slider-min' => $min,
        'data-slider-max' => $max,
        'data-slider-name' => $this->field->name,
        'data-slider-values' => "[$current_min,$current_max]",
        'data-slider-decimals' => $this->decimals,
        'data-slider-thousandsep' => $this->thousands_separator,
        'data-slider-decimalsep' => $this->decimals_separator,
      ),
    );

    // If should add user input.
    if ($this->user_input) {
      $widget['custom_range'] = drupal_get_form('findastic_custom_range_form_' . $this->id, $this->field, $current_min, $current_max, $this->display_suffix);
    }
    else {
      $widget['slider']['range']['from']['#markup'] = '<span class="findastic-slider-from">' . t('From') . ' <strong></strong>' . '<span class="suffix">' . $this->display_suffix . '</span></span>';
      $widget['slider']['range']['to']['#markup'] = '<span class="findastic-slider-to">' . t('To') . ' <strong></strong>' . '<span class="suffix">' . $this->display_suffix . '</span></span>';
    }

    return $widget;
  }
}
