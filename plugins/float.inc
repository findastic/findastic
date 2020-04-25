<?php

class FindasticFieldFloat extends FindasticFieldNumber {

  // Set the index prefix for this field.
  protected $indexPrefix = 'ft';

  /**
   * Returns the cardinality of this field.
   */
  public function cardinality() {
    return 1;
  }

  /**
   * Returns the value of this field.
   */
  public function render($wrapper) {
    $value = parent::render($wrapper);

    // If value is array then return only the first result.
    if (is_array($value)) {
      return array_shift($value);
    }

    return $value;
  }

}
