<?php

class FindasticFieldBoolean extends FindasticField {

  // Set the index prefix for this field.
  protected $indexPrefix = 'b';

  /**
   * Builds the solr query fields.
   * @see FindasticSolrService::search()
   */
  public function queryFields() {
    return array();
  }
}
