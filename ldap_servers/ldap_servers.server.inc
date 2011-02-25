<?php

/**
 * @file
 * Defines server classes and related functions.
 *
 */

/**
 * LDAP Server Class
 *
 *  This class is used to create, work with, and eventually destroy ldap_server
 * objects.
 */
class ldap_server {
  // LDAP Settings

  const LDAP_CONNECT_ERROR = 0x5b;
  const LDAP_SUCCESS = 0x00;
  const LDAP_OPERATIONS_ERROR = 0x01;
  const LDAP_PROTOCOL_ERROR = 0x02;

  public $sid;
  public $name;
  public $status;
  public $type;
  public $address;
  public $port = 389;
  public $tls = FALSE;
  public $basedn;
  private $binddn = FALSE; // Default to an anonymous bind.
  private $bindpw = FALSE; // Default to an anonymous bind.
  public $user_attr;
  public $mail_attr;
  public $testing_drupal_username;

  protected $connection;
  // direct mapping of db to object properties
  private $directly_mapped_properties = array('sid', 'name', 'status', 'type', 'address', 'port',
    'tls', 'basedn', 'binddn', 'bindpw', 'user_attr', 'mail_attr', 'testing_drupal_username');

  /**
   * Constructor Method
   */
  function __construct($sid) {
    if (!is_scalar($sid)) {
      return;      
    }

    $this->sid = $sid;

    $select = db_select('ldap_servers', 'ldap_servers');
    $select->fields('ldap_servers');
    $select->condition('ldap_servers.sid',  $this->sid);


    $server_record = $select->execute()->fetchAllAssoc('sid',  PDO::FETCH_ASSOC);
    //print "<pre>"; print_r($server_record); die;
    foreach ($this->directly_mapped_properties as $property) {
      if (isset($server_record[$this->sid][$property])) {
        $this->{$property} = $server_record[$this->sid][$property];
      }
    }
  }

  /**
   * Destructor Method
   */
  function __destruct() {
    // Close the server connection to be sure.
    $this->disconnect();
  }


  /**
   * Invoke Method
   */
  function __invoke() {
    $this->connect();
    $this->bind();
  }

  /**
   * Error Handling Method
   *
   * @param int errno
   *   The level of the error raised.
   *
   * @param string errstr
   *   The error message.
   *
   * @param string errfile
   *   The filename that the error was raised in.
   *
   * @param int errline
   *   The line number the error was raised at.
   *
   * @param array errcontext
   *   An array of every variable that existed in the scope the error was 
   *   triggered in.
   *
   * @return bool
   *   Always return TRUE to avoid PHP's builtin handler.
   */
  function error_handler($errno, $errstr, $errfile, $errline, $errcontext) {
    return TRUE;
  }


  /**
   * Connect Method
   */
  function connect() {

    //return LDAP_CONNECT_ERROR;
   // print $this->address;
    if (!$con = ldap_connect($this->address, $this->port)) {
      watchdog('user', 'LDAP Connect failure to ' . $this->address . ':' . $this->port);
      return LDAP_CONNECT_ERROR;
    }

    ldap_set_option($con, LDAP_OPT_PROTOCOL_VERSION, 3);
    ldap_set_option($con, LDAP_OPT_REFERRALS, 0);

    // Use TLS if we are configured and able to.
    if ($this->tls) {
      ldap_get_option($con, LDAP_OPT_PROTOCOL_VERSION, $vers);
      if ($vers == -1) {
        watchdog('user', 'Could not get LDAP protocol version.');
        return LDAP_PROTOCOL_ERROR;
      }
      if ($vers != 3) {
        watchdog('user', 'Could not start TLS, only supported by LDAP v3.');
        return LDAP_CONNECT_ERROR;
      }
      elseif (!function_exists('ldap_start_tls')) {
        watchdog('user', 'Could not start TLS. It does not seem to be supported by this PHP setup.');
        return LDAP_CONNECT_ERROR;
      }
      elseif (!ldap_start_tls($con)) {
        $msg =  t("Could not start TLS. (Error %errno: %error).", array('%errno' => ldap_errno($con), '%error' => ldap_error($con)));
        watchdog('user', $msg);
        return LDAP_CONNECT_ERROR;
      }
    }

  // Store the resulting resource
  $this->connection = $con;
  return LDAP_SUCCESS;
  }


  /**
	 * Bind (authenticate) against an active LDAP database.
	 * 
	 * @param $userdn
	 *   The DN to bind against. If NULL, we use $this->binddn
	 * @param $pass
	 *   The password search base. If NULL, we use $this->bindpw 
   *
   * @return
   *   Result of bind; TRUE if successful, FALSE otherwise.
   */
  function bind($userdn = NULL, $pass = NULL) {
    $userdn = ($userdn != NULL) ? $userdn : $this->binddn;
    $pass = ($pass != NULL) ? $pass : ldap_servers_decrypt($this->bindpw);

    // Ensure that we have an active server connection.
    if (!$this->connection) {
      watchdog('ldap', "LDAP bind failure for user %user. Not connected to LDAP server.", array('%user' => $userdn));
      return LDAP_CONNECT_ERROR;
    }

    if (!@ldap_bind($this->connection, $userdn, $pass)) {
      watchdog('ldap', "LDAP bind failure for user %user. Error %errno: %error", array('%user' => $userdn, '%errno' => ldap_errno($this->connection), '%error' => ldap_error($this->connection)));
      return ldap_errno($this->connection);
    }

    return LDAP_SUCCESS;
  }

  /**
   * Disconnect (unbind) from an active LDAP server.
   */
  function disconnect() {
    if (!$this->connection) {
      // never bound or not currently bound, so no need to disconnect
      //watchdog('ldap', 'LDAP disconnect failure from '. $this->server_addr . ':' . $this->port);
    } 
    else {
       ldap_unbind($this->connection);
       $this->connection = NULL;
    }
  }

  /**
   * Preform an LDAP search.
   *
   * @peram string $filter
   *   The search filter.
   * @peram strign $basedn
   *   The search base. If NULL, we use $this->basedn
   * @peram array $attributes
   *   List of desired attributes. If omitted, we only return "dn".
   *
   * @return
   *   An array of matching entries->attributes, or FALSE if the search is
   *   empty.
   */
  function search($filter, $basedn = NULL, $attributes = array()) {
    $basedn = ($basedn != NULL) ? $basedn : $this->basedn;
    $result = ldap_search($this->connection, $basedn, $filter, $attributes);
   // restore_error_handler();
    if ($result && ldap_count_entries($this->connection, $result)) {
      return ldap_get_entries($this->connection, $result);
    }
  
    return $result;
  }



  /**
   * Queries LDAP server for the user.
   *
   * @param $drupal_user_name
   *  drupal user name.
   *
   * @return
   *   An array with users LDAP data or NULL if not found.
   */
  function user_lookup($drupal_user_name) {

    foreach (explode("\r\n", $this->basedn) as $basedn) {
      if (empty($basedn)) continue;
  
      $filter = $this->user_attr . '=' . $drupal_user_name;
    //   print "filter";
    //   print_r($filter);
    //  print "basedn = $basedn";
      $result = $this->search($filter, $basedn);
   //   print "result="; print_r($result); die;
      if (!$result) continue;
  
      // Must find exactly one user for authentication to.
      if ($result['count'] != 1) {
        watchdog('ldapauth', "Error: %num_matches users found with $%filter under %basedn.", array('%num_matches' => $result['count'], '%filter' => $filter, '%basedn' => $basedn), array(), WATCHDOG_ERROR);
        continue;
      }
      $match = $result[0];
  
      // These lines serve to fix the attribute name in case a
      // naughty server (i.e.: MS Active Directory) is messing the
      // characters' case.
      // This was contributed by Dan "Gribnif" Wilga, and described
      // here: http://drupal.org/node/87833
      $name_attr = $this->user_attr;
      if (!isset($match[$name_attr][0])) {
        $name_attr = drupal_strtolower($name_attr);
        if (!isset($match[$name_attr][0]))
          continue;
      }
      // Finally, we must filter out results with spaces added before
      // or after, which are considered OK by LDAP but are no good for us
      // We allow lettercase independence, as requested by Marc Galera
      // on http://drupal.org/node/97728
      //
      // Some setups have multiple $name_attr per entry, as pointed out by
      // Clarence "sparr" Risher on http://drupal.org/node/102008, so we
      // loop through all possible options.
      foreach ($match[$name_attr] as $value) {
        if (drupal_strtolower(trim($value)) == drupal_strtolower($drupal_user_name)) {
          $result = array(
            'dn' =>  $match['dn'],
            'mail' => $match[$this->mail_attr][0],
            'attr' => $match,
          );
          return $result;


        }
      }
    }
  }
}

/**
 * LDAP Server Functions
 *
 *   These functions operate on the server class while not quite fitting
 *   within the class.
 */


// vim:fenc=utf-8:ft=php:ai:si:ts=2:sw=2:et: