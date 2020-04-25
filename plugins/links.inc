<?php

class FindasticWidgetLinks extends FindasticWidget {

  protected $defaults = array(
    'link_type' => 'checkboxes',
  );

  /**
   * Builds the settings form for this widget.
   */
  function settingsForm(&$form, &$form_state) {
    $form['link_type'] = array(
      '#type' => 'select',
      '#title' => t('Link Type'),
      '#options' => array('checkboxes' => t('Checkboxes'), 'radios' => t('Radios')),
      '#default_value' => $this->link_type,
      '#required' => TRUE,
    );
  }

  /**
   * Returns the current widget value.
   */
  public function value() {
    $values = array();

    if (isset($_GET[$this->field->name])) {
      $separator = variable_get('findastic_separator', '-and-');
      $parameters = explode($separator, $_GET[$this->field->name]);
      foreach ($parameters as $value) {
        $values[] = _findastic_transliteration($value, TRUE);
      }
    }

    return $values;
  }

  /**
   * Adds current value to a url.
   */
  function addFacetValueToURL($value = FALSE) {
    if ($this->link_type == 'checkboxes') {
      $values = $this->value;

      if (!in_array($value, $values)) {
        $values[] = $value;
      }
      else {
        $values = array_diff($values, array($value));
      }

      return array($this->field->name => array_map("_findastic_transliteration", array_filter($values)));
    }
    else {
      if ($value) {
        return array($this->field->name => _findastic_transliteration($value));
      }

      return array();
    }
  }

  /**
   * Builds the facet query for this widget type.
   */
  public function facetQuery(&$payload) {
    $payload['params']['facet.field'][] = "{!ex={$this->facetName}}{$this->facetName}";
    $payload['params']['facet.field'][] = "{!ex=filter key={$this->facetName}_default}{$this->facetName}";

    // Add query values if any.
    $query = array();
    foreach ($this->value as $value) {
      $query[] = "{$this->facetName}:\"{$value}\"";
    }

    if (!empty($query)) {
      $payload['params']['fq'][] = "{!tag={$this->facetName},filter}" . implode(' OR ', $query);
    }
  }

  /**
   * Prepares the result from solr search
   */
  public function prepareResult(&$result, $search) {
    $result->facets_default[$this->field->name] = isset($search['facet_counts']['facet_fields'][$this->facetName . '_default']) ? array_filter($search['facet_counts']['facet_fields'][$this->facetName . '_default']) : array();
    $result->facets[$this->field->name] = isset($search['facet_counts']['facet_fields'][$this->facetName]) ? $search['facet_counts']['facet_fields'][$this->facetName] : array();
  }

  /**
   * Adds some json data to the response.
   */
  public function json($result) {
    $field_name = $this->field->name;
    $default_facets = $result->facets_default[$field_name];

    $facet = array();
    foreach ($result->facets[$field_name] as $value => $num) {
      if (array_key_exists($value, $default_facets)) {
        $facet[] = array(
          // Title
          't' => $value,
          // URL
          'u' => ($num > 0) ? url($result->path, array('query' => $this->facetURL($value))) : '#',
          // NUM
          'n' => $num,
          // Checked
          'c' => (bool) in_array($value, $this->value),
          // Term identifier
          'i' => $field_name . '-' . str_replace("-", " ", strtolower(transliteration_get($value))),
          // Field name
          'f' => $field_name,
          // Value
          'v' => _findastic_transliteration($value),
        );
      }
    }
    return $facet;
  }

  /**
   * Renders a clear link.
   */
  public function clearLinks($result) {
    $items = array();

    foreach ($this->value as $value) {
      $query = $this->facetURL($value);
      if ($this->link_type == 'radios') {
        unset($query[$this->field->name]);
      }
      $items[] =  theme('link', array('text' => $value, 'path' => $result->path, 'options' => array(
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
  public function render($result) {
    $widget = array();
    $items = array();
    $facets = array();
    $active = FALSE;

    // Store field name in local variable for better performance.
    $field_name = $this->field->name;
    // If this field is in facets.
    if (isset($result->facets) && array_key_exists($field_name, $result->facets)) {
      // Try to get default facet values.
      $default_facets = $result->facets_default[$field_name];
      // Foreach facet create the link.
      foreach ($result->facets[$field_name] as $value => $num) {
        $classes = array();
        // If num is less than zero then set it to zero
        // and add hidden class.
        if (!array_key_exists($value, $default_facets)) {
          $classes[] = 'hidden';
        }
        else {
          $active = TRUE;
        }

        $path = $result->path;
        $query = $this->facetURL($value);

        $options = array(
          'query' => $query,
          'attributes' => array(
            'rel' => 'nofollow',
          ),
          'html' => TRUE,
        );

        // Check if selected.
        if (in_array($value, $this->value)) {
          $options['attributes']['class'][] = 'checked';
        }

        // if num is greater than zero then its active.
        if ($num > 0) {
          $options['attributes']['class'][] = 'active';
        }

        $context = array(
          'field' => clone $this->field,
        );

        drupal_alter('findastic_facet_link_options', $options, $context);

        $items[] = array(
          'data' => theme('link', array('text' => $value . '<span> (' . $num . ')</span>', 'path' => $path, 'options' => $options)),
          'class' => $classes,
        );

        if (array_key_exists($value, $default_facets)) {
          $facets[] = array(
            // Title
            't' => htmlspecialchars_decode($value),
            // URL
            'u' => ($num > 0) ? url($result->path, array('query' => $options['query'])) : '#',
            // NUM
            'n' => $num,
            // Checked
            'c' => (bool) in_array($value, $this->value),
            // Term identifier
            'i' => $field_name . '-' . str_replace("-", " ", strtolower(transliteration_get($value))),
            // Field name
            'f' => $field_name,
            // Value
            'v' => _findastic_transliteration($value),
            // Rel
            'r' => isset($options['attributes']['rel']) ? $options['attributes']['rel'] : FALSE,
          );
        }
      }
    }

    if (!empty($items) and $active) {
      $widget['facets'] = array(
        '#weight' => -50,
        '#theme' => 'item_list__facet',
        '#facet' => $this->field,
        '#items' => $items,
        '#type' => 'ul',
        '#attached' => array(
          'library' => array(
            array('findastic', 'findastic'),
          ),
          'js' => array(
            array(
              'type' => 'setting',
              'data' => array(
                'findastic' => array(
                  '__INITIAL_STATE__' => array(
                    'facets' => array(
                      "_$this->id" => $facets
                    ),
                  ),
                ),
              ),
            ),
          ),
        ),
        '#attributes' => array(
          'id' => 'findastic-facet-' . $this->id,
          'data-id' => $this->id,
          'data-name' => $field_name,
          'class' => array(
            'findastic-facet',
            $field_name,
          ),
        ),
      );
    }

    return $widget;
  }
}
