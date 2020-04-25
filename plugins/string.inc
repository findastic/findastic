<?php

class FindasticFieldString extends FindasticField {

  /**
   * Builds the solr query fields.
   * @see FindasticSolrService::search()
   */
  public function queryFields() {
    $fields = parent::queryFields();

    // Add ngram query.
    $fields[] = "ton{$this->suffix}^" . ($this->boost / 2);

    // Add no_whitespace query.
    $fields[] = "tonw{$this->suffix}^" . ($this->boost / 2);

    // If query greek language field.
    if (strstr($this->queryName, "_el_")) {
      // Search for greeklish.
      $fields[] = "tog{$this->suffix}^" . ($this->boost / 2);

      // Search for misspell.
      $fields[] = "tom{$this->suffix}^" . ($this->boost / 4);
    }

    return $fields;
  }

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
    return 'to' . $this->suffix;
  }

}
