<?php
/**
 * @file
 * Ebart tools file with administrator only pages
 */

/**
 * Page callback: Ebart tools settings
 *
 * @see ebtools_menu()
 */
function ebtools_form($form, &$form_state) {
  $form['ebtools_name'] = array(
    '#type' => 'textfield',
    '#title' => t('Server name'),
    '#default_value' => variable_get('ebtools_name'),
    '#size' => 32,
    '#maxlength' => 32,
    '#description' => t('Name/identification for server settings'),
    '#required' => TRUE,
  );
  $form['ebtools_login_url'] =  array(
    '#type' => 'textfield',
    '#title' => t('login adress URL'),
    '#default_value' => variable_get('ebtools_login_url', 'http://web.arhiv.rs/names.nsf?Login'),
    '#size' => 64,
    '#maxlength' => 128,
    '#description' => t('URL address for server login'),
    '#required' => TRUE,
  );
  $form['ebtools_access_url'] =  array(
    '#type' => 'textfield',
    '#title' => t('URL address'),
    '#default_value' => variable_get('ebtools_access_url', 'http://web.arhiv.rs/Develop/agentura.nsf/'),
    '#size' => 64,
    '#maxlength' => 128,
    '#description' => t('URL address of database we are accessing the data from'),
    '#required' => TRUE,
  );
  
  $form['ebtools_username'] =  array(
    '#type' => 'textfield',
    '#title' => t('Username'),
    '#default_value' => variable_get('ebtools_username'),
    '#size' => 32,
    '#maxlength' => 32,
    '#description' => t('Username for server access'),
  );

  $form['ebtools_password'] =  array(
    '#type' => 'password',
    '#title' => t('Password'),
    '#default_value' => variable_get('ebtools_password'),
    '#size' => 32,
    '#maxlength' => 32,
  );
  $form['ebtools_start_year'] = array(
    '#type' => 'textfield',
    '#title' => t('Starting year'),
    '#default_value' => variable_get('ebtools_start_year', '2003'),
    '#size' => 4,
    '#maxlength' => 4,
  );
  $form['ebtools_end_year'] = array(
    '#type' => 'textfield',
    '#title' => t('End year'),
    '#default_value' => variable_get('ebtools_end_year', (string) date('Y')),
    '#size' => 4,
    '#maxlength' => 4,
  );

  $form['ebtools_count'] = array(
    '#type' => 'textfield',
    '#title' => t('Max number of results'),
    '#default_value' => variable_get('ebtools_count', 10),
    '#size' => 2,
    '#maxlength' => 2,
    '#description' => t('Maximum number of results to show on page'),
    '#required' => TRUE,
  );
  $form['ebtools_bcount'] = array(
    '#type' => 'textfield',
    '#title' => t('Max number of results in a block'),
    '#default_value' => variable_get('ebtools_bcount', 10),
    '#size' => 2,
    '#maxlength' => 2,
    '#description' => t('Maximum number of results to show when in block'),
    '#required' => TRUE,
  );
  $form['ebtools_tablewidth'] = array(
    '#type' => 'textfield',
    '#title' => t('Number of columns in table view'),
    '#default_value' => variable_get('ebtools_tablewidth', 3),
    '#size' => 2,
    '#maxlength' => 2,
    '#description' => t('Number of columns when showing listing as a table'),
    '#required' => TRUE,
  );
  return system_settings_form($form);
}

