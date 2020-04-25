<?php

/**
 * FindasticField class.
 */
class FindasticComponent implements JsonSerializable {

  // Holds the Findastic results.
  protected static $result;

  // Holds the Findastic query.
  protected static $query;

  // Cache for this object.
  protected $cache;

  /**
   * Field class constructor.
   */
  function __construct(stdClass $result, $query) {
    self::$result = $result;
    self::$result->query = $query;
  }

  /**
   * Magic getter function to return protected variables.
   */
  public function __get($property) {
    if (strpos($property, "_") === 0) {
      $property = trim($property, '_');

      if (array_key_exists($property, self::$result)) {
        return self::$result->{$property};
      }

      return FALSE;
    }

    // If property exists.
    if (property_exists($this, $property)) {
      return $this->{$property};
    }

    // If method with this name exists then execute it and return the result.
    else if (method_exists($this, $property)) {
      return $this->{$property}();
    }

    return false;
  }

  /**
   * Serialize json to specific format.
   */
  public function jsonSerialize() {
    return (String) $this;
  }

  /**
   * Renders the component.
   */
  public function __toString() {
    if (!isset($this->cache['rendered'])) {
      $this->cache['rendered'] = $this->render();
    }
    return $this->cache['rendered'];
  }

  /**
   * Function to render the component.
   */
  function render() {
    return '';
  }
}
