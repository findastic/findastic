<?php

class FindasticFieldText extends FindasticField {
  // Default index prefix is text.
  protected $indexPrefix = 't';

  /**
   * Returns the current field's name suffix.
   */
  protected function suffix() {
    if (!isset($this->suffix)) {
      $this->suffix = ($this->cardinality == 1 ? 's' : 'm') . '_' . ($this->multilingual ? $GLOBALS['language']->language : language_default()->language) . '_' . $this->id;
    }

    return $this->suffix;
  }
}
