<?php

class FindasticFilters extends FindasticComponent {

  /**
   * Renders the current component.
   */
  function render() {
    $items = array();

    // Build clear path.
    $widget = new FindasticWidget();
    $clear_all_path = variable_get('findastic_clear_term', TRUE) ? 'search/' . variable_get('findastic_search_path', 'products') : $this->_path;

    // Add category filter.
    if (isset($this->_query['tid']) and is_string($this->_query['tid']) and variable_get('findastic_clear_term', TRUE)) {
      $term = taxonomy_term_load($this->_query['tid']);
      $vocabulary = taxonomy_vocabulary_load($term->vid);
      $items[] = theme('link', array('text' => t($vocabulary->name) . ': ' . $term->name, 'path' => 'search/' . variable_get('findastic_search_path', 'products'), 'options' => array(
        'query' => $widget->facetURL(),
        'html' => TRUE,
        'attributes' => array(
          'rel' => 'nofollow',
        ),
      )));
    }

    // Add search filter.
    if (isset($this->_query['search'])) {
      $search_text = check_plain($this->_query['search']);
      $widget = new FindasticWidget();
      $search_query = $widget->facetURL();
      unset($search_query['search']);
      $search_path = (strpos($this->_path, "search/") === 0) ? 'search/' . variable_get('findastic_search_path', 'products') : $this->_path;
      $items[] = theme('link', array('text' => t('Search: @term', array('@term' => $search_text)), 'path' => $search_path, 'options' => array(
        'query' => $search_query,
        'html' => TRUE,
        'attributes' => array(
          'rel' => 'nofollow',
        ),
      )));
    }

    // Add facet filters.
    foreach (findastic_get_fields() as $field) {
      if ($field->facet) {
        $links = $field->widget->clearLinks(self::$result);
        foreach ($links as $link) {
          $items[] = $link;
        }
      }
    }

    // Add clear all link.
    if (count($items) > 1) {
      $clear_all_link = theme('link', array('text' => t('Clear All'), 'path' => $clear_all_path, 'options' => array(
        'query' => $widget->facetURL(FALSE, TRUE),
        'html' => TRUE,
        'attributes' => array(
          'rel' => 'nofollow',
          'class' => array('findastic-clear-all'),
        ),
      )));
      array_unshift($items, $clear_all_link);
    }

    return !empty($items) ? theme('item_list', array('items' => $items, 'attributes' => array('class' => array('findastic-filters')))) : '';
  }
}
