<?php

class FindasticWidget {

  // Plugin information.
  protected $plugin = NULL;

  // The related field.
  protected $field = NULL;

  // The widget default settings.
  protected $defaults = array();

  // Caching array for this object.
  private $_cache = array();

  /**
   * Widget constructor.
   */
  function __construct($plugin = FALSE, $field = FALSE) {
    $this->plugin = $plugin;
    $this->field = $field;
  }

  /**
   * Magic getter function to return protected variables.
   */
  public function __get($property) {
    if (!isset($this->_cache[$property])) {
      $this->_cache[$property] = FALSE;

      // If property exists in plugin array return its value.
      if ($this->plugin and array_key_exists($property, $this->plugin)) {
        $this->_cache[$property] = $this->plugin[$property];
      }

      // If property exists in widget settings array return its value.
      else if (isset($this->data['widget']['settings'][$property])) {
        $this->_cache[$property] = $this->data['widget']['settings'][$property];
      }

      // If property exists in plugin array return its value.
      else if (isset($this->defaults[$property])) {
        $this->_cache[$property] = $this->defaults[$property];
      }

      // If property exists.
      else if (property_exists($this, $property)) {
        $this->_cache[$property] = $this->{$property};
      }

      // If method with this name exists then execute it and return the result.
      else if (method_exists($this, $property)) {
        $this->_cache[$property] = $this->{$property}();
      }

      else if ($this->field) {
        $this->_cache[$property] = $this->field->{$property};
      }
    }

    return $this->_cache[$property];
  }

  /**
   * Builds the settings form for this widget.
   */
  public function settingsForm(&$form, &$form_state) {
    // No default settings.
  }

  /**
   * Returns the current widget value.
   */
  public function value() {
    return array();
  }

  /**
   * Renders the widget element.
   */
  public function render($result) {
    return array();
  }

  /**
   * Adds some json data to the response.
   */
  public function json($result) {}

  /**
   * Alters the query for facet support.
   */
  public function facetQuery(&$payload) {}

  /**
   * Alters the query for fulltext search.
   */
  public function textQuery(&$payload, &$words) {}

  /**
   * Prepares the result from solr search
   */
  public function prepareResult(&$result, $search) {}

  /**
   * Renders a clear link.
   */
  public function clearLinks($result) {
    return array();
  }

  /**
   * Adds current value to a url.
   */
  function addFacetValueToURL($value = FALSE) {
    return array();
  }

  /**
   * Builds a facet url.
   */
  public function facetURL($value = FALSE, $clear_all = FALSE) {
    $query = array();

    if (isset($_GET['sort'])) {
      $query['sort'] = check_plain($_GET['sort']);
    }

    if (isset($_GET['search'])) {
      $query['search'] = check_plain($_GET['search']);
    }

    if (!$clear_all) {
      // Add field's values.
      $query = array_merge($query, self::_buildQuery());
      $query = $this->addFacetValueToURL($value) + $query;

      // Remove empty values.
      $query = array_filter($query);
    }

    // Join facets separator.
    $separator = variable_get('findastic_separator', '-and-');

    // Prepare multiple values.
    foreach ($query as $key => $value) {
      if (is_array($value)) {
        $query[$key] = implode($separator, $value);
      }
    }

    return $query;
  }

  /**
   * Builds the default facet query for all widgets.
   * We use this static function for better performance.
   */
  public static function _buildQuery() {
    static $_query;

    if (!isset($_query)) {
      $_query = array();

      foreach (findastic_get_fields() as $field) {
        if ($field->facet) {
          $_query = array_merge($_query, $field->widget->addFacetValueToURL());
        }
      }
    }

    return $_query;
  }
}
