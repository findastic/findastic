<?php

class FindasticFieldCode extends FindasticField {

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
   * Returns the current field's name suffix.
   */
  protected function suffix() {
    return ($this->cardinality == 1 ? 's' : 'm') . '_' . ($this->multilingual ? $GLOBALS['language']->language : language_default()->language) . '_' . $this->id;
  }

  /**
   * Returns the query name of this field.
   */
  public function queryName() {
    return 'tos' . $this->suffix;
  }

}
