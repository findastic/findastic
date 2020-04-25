<?php

/**
 * Helper function to be used as a "property info alter" callback.
 */
function _findastic_wrapper_add_all_properties(EntityMetadataWrapper $wrapper, array $property_info) {
  if ($properties = entity_get_all_property_info($wrapper->type())) {
    $property_info['properties'] = $properties;
  }
  return $property_info;
}

/**
 * FindasticField class.
 */
class FindasticField {

  // Whether this field is searchable of not.
  protected $searchable = TRUE;

  // Default index prefix is string.
  protected $indexPrefix = 's';

  // An array that holds the field information.
  private $field = array();

  // Caching array for this object.
  private $_cache = array();

  /**
   * Field class constructor.
   */
  function __construct(array $field) {
    $this->field = $field;
  }

  /**
   * Magic getter function to return protected variables.
   */
  public function __get($property) {
    if (!isset($this->_cache[$property])) {
      $this->_cache[$property] = FALSE;

      // If property exists.
      if (property_exists($this, $property)) {
        $this->_cache[$property] = $this->{$property};
      }

      // If method with this name exists then execute it and return the result.
      else if (method_exists($this, $property)) {
        $this->_cache[$property] = $this->{$property}();
      }

      // If property exists return its value.
      else if (array_key_exists($property, $this->field)) {
        $this->_cache[$property] = $this->field[$property];
      }

      // If property exists in data array.
      else if (array_key_exists($property, $this->field['data'])) {
        $this->_cache[$property] = $this->field['data'][$property];
      }
    }

    return $this->_cache[$property];
  }

  /**
   * Clears the cache of this object.
   * @See findastic_index_entity().
   */
  public function reset() {
    $this->_cache = array();
  }

  /**
   * Returns the raw data stored in the database.
   */
  public function raw() {
    return $this->field;
  }

  /**
   * Returns the label of this field.
   */
  public function label() {
    // If label is not set yet.
    if (!isset($this->field['label'])) {

      // Get entity wrapper.
      $wrapper = $this->entityWrapper($this->entity_type);

      // Get the field's token parts.
      $parts = explode(':', $this->token);

      $label = array();
      foreach ($parts as $part) {
        $wrapper = $wrapper->{$part};
        $info = $wrapper->info();
        $label[] = $info['label'];

        // If cardinality is more than 1 then get the properties of the delta 0.
        if (is_a($wrapper, 'EntityListWrapper') or is_subclass_of($wrapper, 'EntityListWrapper')) {
          $wrapper = $wrapper->get(0);
        }
      }

      // Populate the label property.
      $this->field['label'] = implode(': ', $label);
    }

    // Return the label.
    return $this->field['label'];
  }

  /**
   * Returns the field info of this field.
   */
  public function info() {
    // If info is not set yet.
    if (!isset($this->info)) {

      // Get entity wrapper.
      $wrapper = $this->entityWrapper($this->entity_type);

      // Get the field's token parts.
      $parts = explode(':', $this->token);

      foreach ($parts as $part) {
        $wrapper = $wrapper->{$part};
        $this->info = $wrapper->info();

        // If cardinality is more than 1 then get the properties of the delta 0.
        if (is_a($wrapper, 'EntityListWrapper') or is_subclass_of($wrapper, 'EntityListWrapper')) {
          $wrapper = $wrapper->get(0);
        }
      }
    }

    // Return the info.
    return $this->info;
  }

  /**
   * Returns the cardinality of this field.
   */
  public function cardinality() {
    // If cardinality is not set yet.
    if (!isset($this->cardinality)) {

      // Get entity wrapper.
      $wrapper = $this->entityWrapper($this->entity_type);

      // Get the field's token parts.
      $parts = explode(':', $this->token);

      // Set cardinality fallback.
      $this->cardinality = 1;

      // Foreach part check if field is list.
      foreach ($parts as $part) {
        $wrapper = $wrapper->{$part};

        // If wrapper is list then return unlimited cardinality.
        if (is_a($wrapper, 'EntityListWrapper') or is_subclass_of($wrapper, 'EntityListWrapper')) {
          $this->cardinality = FIELD_CARDINALITY_UNLIMITED;
          break;
        }
      }
    }

    // Return the cardinality.
    return $this->cardinality;
  }

  /**
   * Helper function for creating an entity metadata wrapper appropriate for
   * this index.
   */
  public static function entityWrapper($entity_type = 'node') {
    $info = array();
    $info['property info alter'] = '_findastic_wrapper_add_all_properties';
    $info['property defaults']['property info alter'] = '_findastic_wrapper_add_all_properties';
    return entity_metadata_wrapper($entity_type, NULL, $info);
  }

  /**
   * Returns all available widgets for this field.
   */
  public function widgets() {
    if (!isset($this->_widgets)) {
      $this->_widgets = array();

      // Include ctools plugins.
      ctools_include('plugins');

      foreach (ctools_get_plugins('findastic', 'widgets') as $type => $plugin) {
        if (in_array($this->type, $plugin['fields'])) {
          $class = $plugin['handler']['class'];
          $this->_widgets[$type] = new $class($plugin, $this);
        }
      }
    }

    return $this->_widgets;
  }

  /**
   * Returns the current field's widget.
   */
  public function widget() {
    if (!isset($this->_widget)) {
      $this->_widget = (isset($this->data['widget']['type']) and isset($this->widgets[$this->data['widget']['type']])) ? $this->widgets[$this->data['widget']['type']] : array_values($this->widgets)[0];
    }

    return $this->_widget;
  }

  /**
   * Gets the current value.
   */
  public function value() {
    return $this->widget->value;
  }

  /**
   * Builds the solr request payload.
   * @see FindasticSolrService::search()
   */
  public function search(&$payload) {
    // By default make all non facet fields indexable.
    if (!$this->facet) {
      $payload['params']['fl'][] = $this->indexName;
    }

    // Allow each widget to build different facet query.
    if ($this->facet and $this->widget) {
      $this->widget->facetQuery($payload);
    }
  }

  /**
   * Builds the solr facet query.
   * @see FindasticSolrService::search()
   */
  public function textQuery(&$payload, &$words) {
    if ($this->facet and $this->widget) {
      // Allow each widget to handle the  different facet query.
      $this->widget->textQuery($payload, $words);
    }
  }

  /**
   * Builds the solr query fields.
   * @see FindasticSolrService::search()
   */
  public function queryFields() {
    $fields = array();

    if ($this->searchable) {
      // Add basic query field.
      $fields[] = "{$this->queryName}^{$this->boost}";

      // Add query fields of other languages.
      if ($this->multilingual) {
        $other_language_boost_multiplicator = variable_get('findastic_other_language_boost_muliplicator', 0.25);
        $language_field = "{$this->queryName}^" . ($this->boost * $other_language_boost_multiplicator);

        if (variable_get('findastic_search_all_languages', FALSE)) {
          // Replace default language with other languages.
          foreach (language_list() as $language) {
            $fields[] = str_replace("_{$GLOBALS['language']->language}_", "_{$language->language}_", $language_field);
          }
        }
      }
    }

    return $fields;
  }

  /**
   * Prepares the result from solr search
   * @see FindasticSolrService::search()
   */
  public function prepareResult(&$result, $search) {
    if ($this->facet) {
      // Allow each widget to prepare the result for displaying.
      $this->widget->prepareResult($result, $search);
    }
  }

  /**
   * Returns the current field's name suffix.
   */
  protected function suffix() {
    return ($this->cardinality == 1 ? 's' : 'm') . ($this->multilingual ? '_' . $GLOBALS['language']->language : '') . '_' . $this->id;
  }

  /**
   * Returns the index name of this field.
   */
  public function indexName() {
    return $this->indexPrefix . $this->suffix;
  }

  /**
   * Returns the query name of this field.
   */
  public function queryName() {
    return $this->indexName;
  }

  /**
   * Returns the facet name of this field.
   */
  public function facetName() {
    return $this->indexName;
  }

  /**
   * Returns the sort name of this field.
   */
  public function sortName() {
    return $this->indexName;
  }

  /**
   * Renders the fields' value.
   */
  public function render($wrapper) {
    $parents = explode(':', $this->token);
    return $this->_render($wrapper, $parents);
  }

  /**
   * Renders recursively the field's value.
   */
  protected function _render($wrapper, $parents) {
    // If wrapper is entity store entity_type and entity_wrapper for later use.
    if (is_a($wrapper, 'EntityDrupalWrapper') or is_subclass_of($wrapper, 'EntityDrupalWrapper')) {
      $entity_type = $wrapper->type();
      $entity_wrapper = $wrapper;
    }

    // Ignore field values for unpublished entities.
    if ($entity_type == 'commerce_product' && !$entity_wrapper->status->value()) {
      return FALSE;
    }

    // Foreach parents go deeper.
    foreach ($parents as $i => $parent) {
      $wrapper = $wrapper->{$parent};

      // Create sub array with children.
      $children = array_splice($parents, $i + 1);


      // Update the entity_type.
      if (!empty($children) and (is_a($wrapper, 'EntityDrupalWrapper') or is_subclass_of($wrapper, 'EntityDrupalWrapper'))) {
        $entity_type = $wrapper->type();
        $entity_wrapper = $wrapper;
      }

      // If cardinality is more than 1 then retrieve all deltas.
      if (is_a($wrapper, 'EntityListWrapper') or is_subclass_of($wrapper, 'EntityListWrapper')) {
        $count = $wrapper->count();

        // Go deeper if there are still parents.
        if (!empty($children)) {
          $values = array();

          // If is list of commerce_products and the field is raw.
          if ($this->type == 'raw' and $wrapper->get(0)->type() == 'commerce_product') {
            $default_product = commerce_product_reference_default_product($wrapper->value());
            $wrapper = entity_metadata_wrapper('commerce_product', $default_product);
            return $this->_render($wrapper, $children);
          }

          // Foreach delta try to recruisevly do deeper.
          for ($delta = 0; $delta < $count; $delta++) {
            if ($value = $this->_render($wrapper->get($delta), $children)) {
              $value = is_array($value) ? $value : array($value);
              $values = array_merge($values, $value);
            }
          }

          // Return multiple values.
          return $values;
        }
      }
    }

    // Return values with formatter.
    if ($this->format) {
      $info = $wrapper->info();
      $entity = $entity_wrapper->value();
      $field_name = $info['name'];

      // Render field.
      $element = field_view_field($entity_type, $entity, $field_name, array(
        'label' => 'hidden',
        'type' => $this->format,
        'settings' => $this->formatter_settings ? $this->formatter_settings : field_info_formatter_settings($this->format),
        // Allow hook_field_attach_view_alter to indentify
        // if this field is rendering by findastic.
        'findastic' => TRUE,
      ));

      // if type is raw then display the field as is.
      $output = array();
      if ($this->type == 'raw') {
        $output[] = drupal_render($element);
      }
      else {
        // Render each value separately.
        foreach (element_children($element) as $delta) {
          $output[] = drupal_render($element[$delta]);
        }
      }

      return count($output) > 1 ? $output : reset($output);
    }

    // Get raw property value.
    $value = $wrapper->raw();
    return $value;
  }
}
