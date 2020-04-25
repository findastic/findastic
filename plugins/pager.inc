<?php

class FindasticPager extends FindasticComponent {

  protected function items_per_page() {
    if (isset($_GET['items_per_page']) and in_array($_GET['items_per_page'], findastic_items_per_page_options())) {
      return $_GET['items_per_page'];
    }

    return ($_GET['q'] == 'instant-search') ? variable_get('findastic_instant_search_items_per_page', 12) : variable_get('findastic_items_per_page', 20);
  }

  /**
   * Renders the current component.
   */
  function render() {
    // Build page pagination.
    pager_default_initialize($this->_total, $this->items_per_page);

    // Return the html for the pagination.
    $output = theme('pager__findastic', array('quantity' => 5));
    return $output ? '<div class="findastic-pager">' . $output . '</div>' : '';
  }
}
