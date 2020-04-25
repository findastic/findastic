<?php

class FindasticSort extends FindasticComponent {

  /**
   * Renders the current component.
   */
  function render() {
    $options = array();
    foreach (findastic_sort_settings() as $value => $item) {
      $options[$value] = t($item['title']);
    }

    // If there are sorting options.
    if (!empty($options)) {
      $sort = array(
        '#name' => 'sort',
        '#type' => 'select',
        '#title' => t('Sort by'),
        '#options' => $options,
        '#value' => isset($_GET['sort']) ? check_plain($_GET['sort']) : findastic_sort_default(),
        '#attributes' => array(
          'class' => array('findastic-sort'),
        ),
      );

      return render($sort);
    }

    return '';
  }
}
