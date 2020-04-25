<?php

class Twig_Extension_Findastic extends \Twig_Extension {

  public function getFilters() {
    return array(
      new \Twig_SimpleFilter('entity_view_field', array($this, 'entity_view_field_placeholder'), array('is_safe' => array('html'))),
    );
  }

  public function entity_view_field_placeholder($entity_id, $entity_type, $field_name) {
    return findastic_entity_view_field($entity_type, $entity_id, $field_name);
  }
}
