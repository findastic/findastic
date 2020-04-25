<?php

class FindasticWidgetRange extends FindasticWidget {

  protected $defaults = array(
    'start' => 0,
    'end' => 200,
    'gap' => 20,
    'user_input' => TRUE,
  );

  /**
   * Builds the settings form for this widget.
   */
  function settingsForm(&$form, &$form_state) {
    $form['start'] = array(
      '#type' => 'textfield',
      '#title' => t('Lowest value'),
      '#default_value' => $this->start,
      '#required' => TRUE,
    );

    $form['end'] = array(
      '#type' => 'textfield',
      '#title' => t('Highest value'),
      '#default_value' => $this->end,
      '#required' => TRUE,
    );

    $form['gap'] = array(
      '#type' => 'textfield',
      '#title' => t('Range gap'),
      '#default_value' => $this->gap,
      '#required' => TRUE,
    );

    $form['user_input'] = array(
      '#type' => 'checkbox',
      '#title' => t('Allow user input'),
      '#default_value' => $this->user_input,
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
    return array_filter(array(
      "min-{$name}" => $value ? $value['min'] : $values['min'],
      "max-{$name}" => $value ? $value['max'] : $values['max'],
    ));
  }

  /**
   * Builds the facet query for this widget type.
   */
  public function facetQuery(&$payload) {
    $payload['params']['facet.range'][] = "{!ex={$this->facetName}}{$this->facetName}";
    $payload['params']["f.{$this->facetName}.facet.range.start"] = $this->start;
    $payload['params']["f.{$this->facetName}.facet.range.end"] = $this->end;
    $payload['params']["f.{$this->facetName}.facet.range.gap"] = $this->gap;

    // Add stats.
    $payload['params']['stats'] = "true";
    $payload['params']['stats.field'][] = "{!ex={$this->facetName}}{$this->facetName}";

    // Add min value.
    if (!empty($this->value['min'])) {
      $payload['params']['fq'][] = "{!tag={$this->facetName}}{$this->facetName}:[{$this->value['min']} TO *]";
    }

    // Add max value.
    if (!empty($this->value['max'])) {
      $payload['params']['fq'][] = "{!tag={$this->facetName}}{$this->facetName}:[* TO {$this->value['max']}]";
    }
  }

  /**
   * Prepares the result from solr search
   */
  public function prepareResult(&$result, $search) {
    $result->facets[$this->field->name] = $search['facet_counts']['facet_ranges'][$this->facetName];
    $stats = $search['stats']['stats_fields'][$this->facetName];
    $stats['min'] = floor($stats['min']);
    $stats['max'] = ceil($stats['max']);
    $result->stats[$this->field->name] = $stats;
  }

  /**
   * Renders the widget element.
   */
  public function render($result) {
    $widget = array();

    $min = $result->stats[$this->field->name]['min'];
    $max = $result->stats[$this->field->name]['max'];
    $values = $this->value;
    $current_min = isset($values['min']) ? $values['min'] : $min;
    $current_max = isset($values['max']) ? $values['max'] : $max;

    $range = $result->facets[$this->field->name];
    $items = array();
    foreach ($range['counts'] as $min => $num) {
      $min = (float) $min;
      $max = $min + $range['gap'];
      $path = $result->path;
      $query = $this->facetURL(array('min' => $min, 'max' => $max));

      $options = array(
        'query' => $query,
        'attributes' => array(
          'rel' => 'nofollow',
          'class' => array('active'),
          'data-value' => $min . '-' . $max,
        ),
        'html' => TRUE,
      );

      // Check if selected.
      if ($this->value['min'] == $min and $this->value['max'] == $max) {
        $options['attributes']['class'][] = 'checked';
      }

      if ($num == 0) {
        $options['attributes']['class'][] = 'hidden';
      }

      $items[] = theme('link', array('text' => $min . ' - ' . $max . '<span> (' . $num . ')</span>', 'path' => $path, 'options' => $options));
    }

    if (!empty($items)) {
      $widget['facets'] = array(
        '#weight' => -50,
        '#theme' => 'item_list__facet',
        '#items' => $items,
        '#type' => 'ul',
        '#attached' => array(
          'library' => array(
            array('findastic', 'findastic'),
          ),
        ),
        '#attributes' => array(
          'id' => 'findastic-facet-' . $this->id,
          'class' => array(
            'findastic-facet',
          ),
        ),
      );
    }

    // If should add user input.
    if ($this->user_input) {
      $widget['custom_range'] = drupal_get_form('findastic_custom_range_form_' . $this->id, $this->field, $current_min, $current_max);
    }

    return $widget;
  }
}
