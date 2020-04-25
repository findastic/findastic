<?php

/**
 * Class to create, update, delete and search documents in Solr.
 */
class FindasticSolrService {

  protected $_offlineMessage = '';

  /**
   * Static instance creator to implement singleton design pattern.
   */
  public static function getInstance() {
    static $instance;

    if (!isset($instance)) {
      $class = static::class;
      $instance = new $class();
    }

    return $instance;
  }

  /**
   * Constructor.
   */
  public function __construct() {
    // Get default service settings.
    $this->settings = variable_get($this->name(), array()) + $this->defaults();
    $this->hash = variable_get('findastic_hash_salt', drupal_get_hash_salt());
  }

  /**
   * Returns a unique machine name for this class.
   */
  public function name() {
    // De-camelcase and return the class name.
    return trim(strtolower(preg_replace('/[A-Z]+/', "-$0", get_class($this))), '-');
  }

  /**
   * Returns the service name.
   */
  public function label() {
    return t('Apache Solr');
  }

  /**
   * Returns the current service settings.
   */
  public function settings($name = FALSE) {
    // If a specific setting is asked then return single value.
    if ($name) {
      return isset($this->settings[$name]) ? $this->settings[$name] : '';
    }

    // Else return all settings.
    return $this->settings;
  }

  /**
   * Return the default settings.
   */
  protected function defaults() {
    return array(
      'host' => 'http://localhost:8983',
      'core' => 'core1',
    );
  }

  /**
   * Handles the findastic settings form.
   */
  public function settingsForm(&$form, &$form_state) {
    // Solr host.
    $form['host'] = array(
      '#type' => 'textfield',
      '#title' => t('Host'),
      '#description' => 'http://USERNAME:PASSWORD@HOST:PORT',
      '#default_value' => $this->settings('host'),
      '#required' => TRUE,
    );

    // Solr core.
    $form['core'] = array(
      '#type' => 'textfield',
      '#title' => t('Core'),
      '#default_value' => $this->settings('core'),
      '#required' => TRUE,
    );
  }

  /**
   * Returns current solr schema or error if there is something wrong with
   * the configuration settings.
   */
  protected function schema() {
    // Get solr host and core.
    $host = $this->settings('host');
    $core = $this->settings('core');

    // Try to get schema.xml properties.
    $response = drupal_http_request("{$host}/solr/{$core}/schema");

    // If response code is not 200.
    if ($response->code != 200) {
      throw new Exception($response->error, $response->code);
    }

    // Parse data.
    $result = drupal_json_decode($response->data);

    // Check if schema name is findastic.
    if ($result['schema']['name'] != 'findastic-' . FINDASTIC_SCHEMA_VERSION) {
      throw new Exception(t('Findastic schema is not loaded or wrong version.'), 1);
    }

    return $result['schema'];
  }

  /**
   * Returns solr connection and configuration status.
   */
  public function status() {
    // If can ping schema then solr is up and running.
    try {
      $this->schema();
    }
    catch (Exception $e) {
      $this->_offlineMessage = $e->getMessage();
      return FALSE;
    }

    return TRUE;
  }

  /**
   * Implementation of interface.
   */
  public function offlineMessage() {
    return $this->_offlineMessage;
  }

  /**
   * Performs a json request to apache solr.
   */
  protected function request($payload, $service = 'query', $method = 'POST') {
    // Get solr host and core.
    $host = $this->settings('host');
    $core = $this->settings('core');

    // Do the request using drupal_http_request method.
    $response = drupal_http_request("{$host}/solr/{$core}/{$service}", array(
      'method' => $method,
      'data' => drupal_json_encode($payload),
      'headers' => array('Content-Type' => 'application/json')
    ));

    // Throw an error if something goes wrong.
    if ($response->code != 200) {
      $message = $response->error;

      // If there is a message from solr ther we prefer to display it.
      if ($response->data) {
        $result = drupal_json_decode($response->data);
        $message = $result['error']['msg'];
      }

      // Throw the error.
      throw new Exception($message, $response->code);
    }

    return drupal_json_decode($response->data);
  }

  /**
   * Updates synonyms for a group.
   */
  public function updateSynonyms($synonyms, $group = 'greek') {
    // Empty the synonym group if exists.
    if ($existing = $this->request(array(), 'schema/analysis/synonyms/' . $group, 'GET')) {
      if (isset($existing['synonymMappings']['managedMap'])) {
        foreach ($existing['synonymMappings']['managedMap'] as $keyword => $items) {
          $this->request(array(), "schema/analysis/synonyms/$group/" . urlencode($keyword), 'DELETE');
        }
      }
    }
    // If synonym group does not exist then create it
    // and set initArgs to ignoreCase.
    else {
      $this->request(array('initArgs' => array('ignoreCase' => TRUE)), 'schema/analysis/synonyms/' . $group);
    }

    // Create payload for updating.
    $payload = array();
    foreach ($synonyms as &$words) {
      $words = array_values(array_filter(array_map(array($this, '_strtolower'), array_map('trim', $words))));
      if (count($words) > 1) {
        foreach ($words as $word) {
          $payload[$word] = $words;
        }
      }
    }

    // Preform the update and return the result.
    return $this->request($payload, 'schema/analysis/synonyms/' . $group);
  }

  /**
   * Adds a document to the index.
   */
  public function index($document) {
    // Add hash to the document.
    $document->hash = $this->hash;
    $document->id = $this->hash . '-' . $document->id;

    // Build the add payload.
    $payload = array(
      'add' => array(
        'doc' => $document,
        'commitWithin' => 2000,
      ),
    );

    // Perform the update and return the result.
    return $this->request($payload, 'update');
  }

  /**
   * Removes a document from the index.
   */
  public function remove($id) {
    // Build the add payload.
    $payload = array(
      'delete' => array(
        'query' => 'id:' . $this->hash . '-' . $id,
        'commitWithin' => 2000,
      ),
    );

    // Perform the delete and return the result.
    return $this->request($payload, 'update');
  }

  /**
   * Removes all documents from the index.
   */
  public function clear($ignore_hash = FALSE) {
    // Build the add payload.
    $payload = array(
      'delete' => array(
        'query' => '*:*',
        'commitWithin' => 2000,
      ),
    );

    if (!$ignore_hash) {
      $payload['delete']['query'] = 'hash:"' . $this->hash . '"';
    }

    return $this->request($payload, 'update');
  }

  /**
   * Searches in Drupal.
   */
  public function search($query = array(), $settings = array()) {
    // Build the payload with the default params.
    $payload = array(
      'params' => $settings + array(
        'json.nl' => 'map',
        'fq' => array(),
        'facet' => 'true',
        'facet.field' => array(),
        'facet.limit' => -1, // unlimited values
        'fl' => array('entity_type', 'entity_id', 'bundle', 'status'),
      ),
    );

    // If text query.
    if (isset($query['search'])) {
      $payload['query'] = '*:*';
      $payload['params']['fq'][] = $this->_query($query, $settings);
      $text_query = $this->_textQuery($query['search'], $payload);

      // If text query not empty or no other filter applied
      // then perform a dismax query.
      if (!empty($text_query) or count($payload['params']['fq']) == 1) {
        $payload['params']['mm'] = variable_get('findastic_search_tolerance', '75%');
        $payload['params']['mm.autoRelax'] = FALSE;
        $payload['params']['qf'] = $this->_queryFields();
        $payload['params']['defType'] = 'edismax';
        $payload['params']['sort'] = 'score desc,' . $payload['params']['sort'];
        $payload['query'] = "{!tag=filter}" . $text_query;
      }
    }
    else {
      // Build the query from array.
      $payload['query'] = $this->_query($query, $settings);
    }

    // Build the facets query.
    foreach (findastic_get_fields() as $field) {
      $field->search($payload);
    }

    // Allow other modules to alter the search query.
    drupal_alter('findastic_search', $payload, $query, $settings);

    return $this->_search($payload);
  }

  /**
   * Processes and returns the query for solr search.
   */
  protected function _query($query = array(), $settings = array()) {
    $expressions = array('hash:"' . $this->hash .'"');

    // Add status expression.
    if (!isset($settings['ignoreStatus'])) {
      $expressions[] = 'status:true';
    }

    foreach ($query as $name => $values) {
      // Skip fulltext search.
      if ($name == 'search') {
        continue;
      }

      // If field name exists then replace the name with the queryName.
      if ($field = findastic_get_field_by_name($name)) {
        $name = $field->queryName;
      }

      // Add the values to the query.
      $values = is_array($values) ? $values : array($values);
      foreach ($values as $value) {
        $expressions[] = strpos($value, ":") === FALSE ? "{$name}:\"{$value}\"" : "($value)";
      }
    }

    return implode(' AND ', $expressions);
  }

  /**
   * Transoforms a text to lower case.
   */
  protected function _strtolower($text) {
    $replacements = array('Ά' => 'α', 'Έ' => 'ε', 'Ή' => 'η', 'Ί' => 'ι', 'Ό' => 'ο', 'Ύ' => 'υ', 'Ώ' => 'ω', 'ά' => 'α', 'έ' => 'ε', 'ή' => 'η', 'ί' => 'ι', 'ό' => 'ο', 'ύ' => 'υ', 'ώ' => 'ω');
    return mb_strtolower(str_replace(array_keys($replacements), $replacements, $text), 'UTF-8');
  }

  /**
   * Prepares the text query.
   */
  protected function _textQuery($text, &$payload) {
    $words = explode(' ', $this->_strtolower($text));

    // Allow each field to alter the payload.
    foreach (findastic_get_fields() as $field) {
      $field->textQuery($payload, $words);
    }

    $not = array_filter(explode("\n", variable_get('findastic_keyword_not', "εκτός\nεκτος\nόχι\nοχι\nχωρίς\nχωρίς\nδίχως\n!\nektos\noxi\nxoris\ndixos\nnot")));
    $stopwords = array_filter(explode("\n", variable_get('findastic_stopwords', "ο\nη\nτο\nοι\nτα\nτου\nτησ\nτων\nτον\nτην\nκαι\nκι\nκ\nειμαι\nεισαι\nειναι\nειμαστε\nειστε\nστο\nστον\nστη\nστην\nμα\nαλλα\nαπο\nγια\nπροσ\nμε\nσε\nωσ\nπαρα\nαντι\nκατα\nμετα\nθα\nνα\nδε\nδεν\nμη\nμην\nεπι\nενω\nεαν\nαν\nτοτε\nπου\nπωσ\nποιοσ\nποια\nποιο\nποιοι\nποιεσ\nποιων\nποιουσ\nαυτοσ\nαυτη\nαυτο\nαυτοι\nαυτων\nαυτουσ\nαυτεσ\nαυτα\nεκεινοσ\nεκεινη\nεκεινο\nεκεινοι\nεκεινεσ\nεκεινα\nεκεινων\nεκεινουσ\nοπωσ\nομωσ\nισωσ\nοσο\nοτι\nαπό\nαπο\na\nan\nand\nare\nas\nat\nbe\nbut\nby\nfor\nif\nin\ninto\nis\nit\nno\nnot\nof\non\nor\nsuch\nthat\nthe\ntheir\nthen\nthere\nthese\nthey\nthis\nto\nwas\nwill\nwith\nευρώ\nευρω\neuro\neuros\nevro\neyro\n$\n€\nπροϊόντα\nπροιόντα\nπροιοντα\nπροϊόν\nπροιόν\nπροιον\nproducts\nproduct")));

    // Apply "not" keyword.
    $not_words = array();
    foreach($words as $i => $word) {
      // If keyword "not" found in search.
      if (in_array($word, $not)) {
        $j = $i;
        do {
          $j++;
          $nextword = isset($words[$j]) ? $words[$j] : FALSE;
        }
        while ($nextword and in_array($nextword, $stopwords));
        if ($nextword) {
          for ($k = $i; $k <= $j; $k++) {
            unset($words[$k]);
          }
          $not_words[] = '-' . $nextword;
        }
      }
    }

    // Remove stopwords.
    $words = array_values(array_diff($words, $stopwords));

    // Search every word that is
    // more than one character.
    $query_words = array();
    $word_count = count($words);
    foreach ($words as $i => $word) {
      $len = mb_strlen($word);

      if ($len > 1) {
        $query_words[] = $word;

        if ($len >= 8) {
          $query_words[] = "$word~2";
        }

        else if ($len >= 5) {
          $query_words[] = "$word~1";
        }
      }
    }

    if (variable_get('findastic_merge_two_words', TRUE)) {
      // If exactly 2 words try merging them.
      if (count($words) == 2) {
        $query_words[] = implode('', $words);
      }
    }

    return implode(' ', array_merge($query_words, $not_words));
  }

  /**
   * Builds the "qf" parameter.
   * What fields will be searched and their boost values.
   */
  protected function _queryFields($query = array()) {
    $qf = array();

    // Foreach field.
    foreach (findastic_get_fields() as $field) {
      if ($fields = $field->queryFields()) {
        $qf = array_merge($qf, $fields);
      }
    }

    return implode(' ', $qf);
  }

  /**
   * Executes the serch prepares the returns result.
   */
  protected function _search($payload) {
    $result = new stdClass();
    $result->total = 0;

    try {
      // Perform the solr search.
      $search = $this->request($payload);

      // Get total count.
      $result->total = $search['response']['numFound'];

      // Prepare items.
      $result->items = array();
      foreach ($search['response']['docs'] as $doc) {
        $result->items[] = $this->_prepareDocument($doc);
      }

      // Prepares the result.
      foreach (findastic_get_fields() as $field) {
        $field->prepareResult($result, $search);
      }

      // Allow other modules to alter the search result.
      drupal_alter('findastic_result', $result, $search);
    }
    catch (Exception $e) {
      if (user_access('administer findastic')) {
        drupal_set_message($e->getMessage(), 'error');
      }
    }

    return $result;
  }

  /**
   * Prepares a document for displaying.
   */
  protected function _prepareDocument($document) {
    // Foreach field replace the indexName with the field name.
    foreach (findastic_get_fields() as $field) {
      $name = $field->indexName;
      if (array_key_exists($name, $document)) {
        $document[$field->name] = $document[$name];
        unset($document[$name]);
      }
    }

    // Remove version field.
    unset($document['_version_']);

    foreach ($document as &$field) {
      $this->_prepareField($field);
    }

    return $document;
  }

  /**
   * Prepares a document field for displaying.
   */
  protected function _prepareField(&$field) {
    // If field is array then recursively prepare field.
    if (is_array($field)) {
      foreach ($field as &$subfield) {
        $this->_prepareField($subfield);
      }
    }
    else {
      // Prepares form token and action url.
      if (strpos($field, '<form') !== FALSE) {
        preg_match_all("/\[form_token\:(.+)?(?=)\]/", $field, $tokens);
        foreach ($tokens[0] as $key => $value) {
          $token = ($GLOBALS['user']->uid > 0) ? '<input type="hidden" name="form_token" value="' . drupal_get_token($tokens[1][$key]) . '"/>' : '';
          $field = str_replace("$value", $token, $field);
        }

        preg_match_all("/action=\"([^\"]+)/", $field, $actions);
        foreach ($actions[0] as $action) {
          $query = drupal_get_query_parameters();
          if (isset($query['_'])) { unset($query['_']); }
          $destination = trim($_GET['q'] . '?' . drupal_http_build_query($query), '?');
          $field = str_replace($action, $action . '?destination=' . urlencode($destination), $field);
        }
      }
      // Prepares the wishlist url
      if (strpos($field, '[wishlist') !== FALSE) {
        preg_match_all("/\[wishlist\:(.+)?(?=)\]/", $field, $tokens);
        foreach ($tokens[0] as $i => $token) {
          $wishlist = flag_create_link('wishlist', $tokens[1][$i]);
          $field = str_replace($token, $wishlist, $field);
        }
      }
    }
  }
}
