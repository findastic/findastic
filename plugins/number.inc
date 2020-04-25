<?php

class FindasticFieldNumber extends FindasticField {

  // Set the index prefix for this field.
  protected $indexPrefix = 'it';

  /**
   * Builds the solr query fields.
   * @see FindasticSolrService::search()
   */
  public function queryFields() {
    return array();
  }

  /**
   * Prepares the result from solr search
   * @see FindasticSolrService::search()
   */
  public function prepareResult(&$result, $search) {
    parent::prepareResult($result, $search);

    // Change the type of facets from string of numeric.
    if (isset($result->facets[$this->name])) {
      foreach ($result->facets[$this->name] as $key => $value) {
        unset($result->facets[$this->name][$key]);
        $result->facets[$this->name][(string) ((float) $key)] = $value;
      }
    }

    // Change the type of default facets from string of numeric.
    if (isset($result->facets_default[$this->name])) {
      foreach ($result->facets_default[$this->name] as $key => $value) {
        unset($result->facets_default[$this->name][$key]);
        $result->facets_default[$this->name][(string) ((float) $key)] = $value;
      }
    }
  }
}
