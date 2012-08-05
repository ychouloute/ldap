<?php
// $Id$

/**
 * @file
 * simpletests for ldap authorization
 *
 */
// require_once(drupal_get_path('module', 'ldap_servers') . '/tests/LdapTestFunctions.class.php');

ldap_server_module_load_include('php', 'ldap_test', 'LdapTestFunctionsv2.class');

class LdapUserTestCasev2 extends DrupalWebTestCase {

  public $module_name = 'ldap_user';
  public $testFunctions;

  // storage for test data
  public $useFeatureData;
  public $featurePath;
  public $featureName;

  public $ldapTestId;
  public $authorizationData;
  public $authenticationData;
  public $testData = array();

  public static function getInfo() {
    return array(
      'name' => 'LDAP User Basic Tests',
      'description' => 'Test ldap user functionality.',
      'group' => 'LDAP User'
    );
  }

  public $sid; // current, or only, sid

  function __construct($test_id = NULL) {
    parent::__construct($test_id);
    $this->testFunctions = new LdapTestFunctionsv2();
  }

  function setUp() {
    parent::setUp(array('ldap_authentication', 'ldap_authorization', 'ldap_authorization_drupal_role', 'ldap_test'));
    variable_set('ldap_simpletest', 2);
    variable_set('ldap_help_watchdog_detail', 0);
    // create field_lname, field_binary_test user fields



  foreach ($this->ldap_user_test_entity_fields() as $field_id => $field_conf) {
    $field_info = field_info_field($field_id);
  //  debug("field: $field_id"); debug($field_info);
    if (!$field_info) {
    //  debug("create field: $field_id");
     // debug($field_conf['field']);
     // debug($field_conf['instance']);
      field_create_field($field_conf['field']);
      field_create_instance($field_conf['instance']);
    }
    $field_info = field_info_field($field_id);
   // debug("created field: $field_id"); debug($field_info);
  }


  }

  function tearDown() {
    parent::tearDown();
    variable_del('ldap_help_watchdog_detail');
    variable_del('ldap_simpletest');
  }

  function prepTestData($sids,
      $ldap_user_conf_id = NULL,
      $ldap_authentication_conf_id = NULL,
      $ldap_authorization_conf_id = NULL,
      $ldap_feeds_conf_id = NULL,
      $ldap_query_conf_id = NULL
    ) {


    $this->testFunctions->configureLdapServers($sids);
    if ($ldap_user_conf_id) {
      $this->testFunctions->configureLdapUser($ldap_user_conf_id);
    }
    if ($ldap_authentication_conf_id) {
      $this->testFunctions->configureLdapAuthentication($ldap_authentication_conf_id, $sids);
    }
  }



  function ldap_user_test_entity_fields() {

    $fields = array();

    $fields['field_lname']['field'] = array(
      'field_name' => 'field_lname',
      'type' => 'text',
      'settings' => array(
        'max_length' => 64,
      )
    );

    $fields['field_lname']['instance'] = array(
      'field_name' => 'field_lname',
      'entity_type' => 'user',
      'label' => 'Last Name',
      'bundle' => 'user',
      'required' => FALSE,
      'widget' => array(
        'type' => 'text_textfield',
      ),
      'display' => array(
        'default' => array(
          'type' => 'text_default',
        ),
      ),
      'settings' => array('user_register_form' => FALSE)
    );

    $fields['field_fname']['field'] = array(
      'field_name' => 'field_fname',
      'type' => 'text',
      'settings' => array(
        'max_length' => 64,
      )
    );

    $fields['field_fname']['instance'] = array(
      'field_name' => 'field_fname',
      'entity_type' => 'user',
      'label' => 'Last Name',
      'bundle' => 'user',
      'required' => FALSE,
      'widget' => array(
        'type' => 'text_textfield',
      ),
      'display' => array(
        'default' => array(
          'type' => 'text_default',
        ),
      ),
      'settings' => array('user_register_form' => FALSE)
    );
    
    // display name for testing compound tokens
    $fields['field_display_name']['field'] = array(
      'field_name' => 'field_display_name',
      'type' => 'text',
      'settings' => array(
        'max_length' => 64,
      )
    );

    $fields['field_display_name']['instance'] = array(
      'field_name' => 'field_display_name',
      'entity_type' => 'user',
      'label' => 'Display Name',
      'bundle' => 'user',
      'required' => FALSE,
      'widget' => array(
        'type' => 'text_textfield',
      ),
      'display' => array(
        'default' => array(
          'type' => 'text_default',
        ),
      ),
      'settings' => array('user_register_form' => FALSE)
    );
    
    // display name for testing compound tokens
    $fields['field_binary_test']['field'] = array(
      'field_name' => 'field_binary_test',
      'type' => 'text',
      'size' => 'big',
    );

    $fields['field_binary_test']['instance'] = array(
      'field_name' => 'field_binary_test',
      'entity_type' => 'user',
      'label' => 'Binary Field',
      'bundle' => 'user',
      'required' => FALSE,
      'widget' => array(
        'type' => 'text_textfield',
      ),
      'display' => array(
        'default' => array(
          'type' => 'text_default',
        ),
      ),
      'settings' => array('user_register_form' => FALSE)
    );
    
    return $fields;

  }

}
