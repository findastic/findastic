<?php

class FindasticFieldRawText extends FindasticField {

  // Html fields are only for display purposes.
  protected $searchable = FALSE;

  // Html should be stored in "content_*" store_only fields.
  protected $indexPrefix = 'r';

  /**
   * Returns TRUE or FALSE whether this field is multilingual.
   */
  protected function multilingual() {
    if (!isset($this->_multilingual)) {
      $languages = language_list('enabled');
      $this->_multilingual = (count($languages[1]) > 1);
    }

    return $this->_multilingual;
  }

  /**
   * Returns the cardinality of this field.
   */
  public function cardinality() {
    // Set cardinality of raw fields always to 1.
    return 1;
  }
}
