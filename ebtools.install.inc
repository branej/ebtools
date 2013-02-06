<?php
/**
 * @file database schema for ebtools module
 */

/**
 * Implements hook_enable()
 */
function ebtools_enable() {
  //Check if table exists, if not install the schema.
  if(db_table_exists('ebtools_server') == FALSE) {
    drupal_install_schema('ebtools');
  }
}

/**
 * Implements hook_install()
 */
#function ebtools_install() {
#  drupal_install_schema('ebtools');
#}

/**
 * Implements hook_uninstall()
 */
#function ebtools_uninstall() {
#  drupal_uninstall_schema('ebtools');
#}

function ebtools_schema() {
  $schema['ebtools_server'] = array(
    'description' => 'Server definition table',
    'fields' => array(
      'sid' => array(
        'description' => 'ID field',
        'type' => 'serial',
        'unsigned' => TRUE,
        'not null' => TRUE,
      ),
      'name' = > array(
        'description' => 'name of the server, must be unique',
        'type' => 'varchar',
        'length' => 32,
        'not null' => TRUE,
        'default' => '',
      ),
      'url' => array(
        'description' => 'base URL of the server',
        'type' => 'varchar',
        'length' => 128,
        'not null' => TRUE,
        'default' => '',
      ),
    ),
    'unique keys' => array(
      'name' => array('name'),
    ),
    'primary key' => array('sid'),
  );
  $schema['ebtools_service'] = array(
    'description' => 'Service definition table',
    'fields' => array(
      'cid' => array(
        'description' => 'ID field',
        'type' => 'serial',
        'unsigned' => TRUE,
        'not null' => TRUE,
      ),
      'name' => array(
        'description' => 'name of the service',
        'type' => 'varchar',
        'length' => 32,
        'not null' => TRUE,
        'default' => '',
      ),
      'suffix' => array(
        'description' => 'url suffix',
        'type' => 'varchar',
        'length' => 128,
        'not null' => TRUE,
        'default' => '',
      ),
      'sid' => array(
        'description' => 'Foreign key referencing {ebtools_server}',
        'type' => 'int',
        'unsigned' => TRUE,
        'not null' => TRUE,
      ),
    ),
    'unique keys' => array(
      'name' => array('name'),
    ),
    'primary key' => array('cid'),
    'foreign keys' => array(
      'ebtools_server' => array(
        'table' => 'ebtools_server',
        'columns' => array('sid' => 'sid'),
      ),
    ),
  );
  $schema['ebtools_variable'] = array(
    'description' => 'Arguments for service',
    'fields' => array(
      'vid' => array(
        'description' => 'ID field',
        'type' => 'serial',
        'unsigned' => TRUE,
        'not null' => TRUE,
      ),
      'name' => array(
        'description' => 'variable name',
        'type' => 'varchar',
        'length' => '16',
        'not null' => TRUE,
        'default' => '',
      ),
      'cid' => array(
        'description' => 'Foreign key referencing {ebtools_service}',
        'type' => 'int',
        'unsigned' => TRUE,
        'not null' => TRUE,
      ),
    ),
      'unique keys' => array(
        'name' => array('name'),
      ),
      'primary key' => array('vid'),
      'foreign keys' => array(
        'ebtools_service' => array(
          'table' => 'ebtools_service',
          'columns' => array('cid' => 'cid'),
        ),
      ),
    ),
  );

  return $schema;  
}

