<?php

class FindasticResults extends FindasticComponent {

  public function total() {
    return $this->_total;
  }

  public function all() {
    return $this->_items;
  }

  public function jsonSerialize() {
    return array(
      'all' => $this->_items,
      'total' => $this->_total,
    );
  }
}
