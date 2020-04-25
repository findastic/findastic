<?php

class FindasticItemsPerPage extends FindasticComponent {

  protected function items_per_page() {
    if (isset($_GET['items_per_page']) and in_array($_GET['items_per_page'], findastic_items_per_page_options())) {
      return $_GET['items_per_page'];
    }

    return ($_GET['q'] == 'instant-search') ? variable_get('findastic_instant_search_items_per_page', 12) : variable_get('findastic_items_per_page', 20);
  }

  protected function element() {
    $element = array(
      '#name' => 'items_per_page',
      '#type' => 'select',
      '#title' => t('Items per page'),
      '#value' => $this->items_per_page,
      '#attributes' => array(
        'class' => array('findastic-items-per-page'),
      ),
    );

    foreach (findastic_items_per_page_options() as $key => $value) {
      $element['#options'][$key] = $value;

      // If option is more than the total then end the loop here.
      if ($key > $this->_total) {
        break;
      }
    }

    // If no options then disable the element.
    if (empty($element['#options'])) {
      $element['#options'] = array($this->_total => $this->_total);
      $element['#value'] = $this->_total;
      $element['#attributes']['disabled'] = TRUE;
    }

    return $element;
  }

  /**
   * Renders the current component.
   */
  function render() {
    $element = $this->element;
    return render($element);
  }
}
