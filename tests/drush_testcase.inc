<?php

/**
 * @file
 *   Initialize a sandboxed environment. Starts with call unish_init() at bottom.
 */

abstract class Drush_TestCase extends PHPUnit_Framework_TestCase {

  function __construct($name = NULL, array $data = array(), $dataName = '') {
    $this->_output = FALSE;
    parent::__construct($name, $data, $dataName);
  }

  /**
   * Assure that each class starts with an empty sandbox directory and
   * a clean environment - http://drupal.org/node/1103568.
   */
  public static function setUpBeforeClass() {
    self::setUpFreshSandBox();
  }

  /**
   * Remove any pre-existing sandbox, then create a new one.
   */
  public static function setUpFreshSandBox() {
    $sandbox = UNISH_SANDBOX;
    if (file_exists($sandbox)) {
      unish_file_delete_recursive($sandbox);
    }
    $ret = mkdir($sandbox, 0777, TRUE);
    chdir(UNISH_SANDBOX);

    mkdir(getenv('HOME') . '/.drush', 0777, TRUE);
    mkdir($sandbox . '/etc/drush', 0777, TRUE);
    mkdir($sandbox . '/share/drush/commands', 0777, TRUE);

    if (strtoupper(substr(PHP_OS, 0, 3)) === 'WIN') {
      // Hack to make git use unix line endings on windows
      // We need it to make hashes of files pulled from git match ones hardcoded in tests
      if (!file_exists($sandbox . '\home')) {
        mkdir($sandbox . '\home');
      }
      exec("git config --file $sandbox\\home\\.gitconfig core.autocrlf false", $output, $return);
    }
  }

  /**
   * Runs after all tests in a class are run. Remove sandbox directory.
   */
  public static function tearDownAfterClass() {
    if (file_exists(UNISH_SANDBOX)) {
      unish_file_delete_recursive(UNISH_SANDBOX, TRUE);
    }
  }

  /**
   * Print a log message to the console.
   *
   * @param string $message
   * @param string $type
   *   Supported types are:
   *     - notice
   *     - verbose
   *     - debug
   */
  function log($message, $type = 'notice') {
    $line = "\nLog: $message\n";
    switch ($this->log_level()) {
      case 'verbose':
        if (in_array($type, array('notice', 'verbose'))) print $line;
        break;
      case 'debug':
        print $line;
        break;
      default:
        if ($type == 'notice') print $line;
        break;
    }
  }

  function log_level() {
    if (in_array('--debug', $_SERVER['argv'])) {
      return 'debug';
    }
    elseif (in_array('--verbose', $_SERVER['argv'])) {
      return 'verbose';
    }
  }

  public static function is_windows() {
    return (strtoupper(substr(PHP_OS, 0, 3)) == "WIN");
  }

  public static function get_tar_executable() {
    return self::is_windows() ? "bsdtar.exe" : "tar";
  }

  /**
   * Converts a Windows path (dir1\dir2\dir3) into a Unix path (dir1/dir2/dir3).
   * Also converts a cygwin "drive emulation" path (/cygdrive/c/dir1) into a
   * proper drive path, still with Unix slashes (c:/dir1).
   *
   * @copied from Drush's environment.inc
   */
  function convert_path($path) {
    $path = str_replace('\\','/', $path);
    $path = preg_replace('/^\/cygdrive\/([A-Za-z])(.*)$/', '\1:\2', $path);

    return $path;
  }

  /**
   * Borrowed from Drush.
   * Checks operating system and returns
   * supported bit bucket folder.
   */
  function bit_bucket() {
    if (!$this->is_windows()) {
      return '/dev/null';
    }
    else {
      return 'nul';
    }
  }

  public static function escapeshellarg($arg) {
    // Short-circuit escaping for simple params (keep stuff readable)
    if (preg_match('|^[a-zA-Z0-9.:/_-]*$|', $arg)) {
      return $arg;
    }
    elseif (self::is_windows()) {
      return self::_escapeshellarg_windows($arg);
    }
    else {
      return escapeshellarg($arg);
    }
  }

  public static function _escapeshellarg_windows($arg) {
    // Double up existing backslashes
    $arg = preg_replace('/\\\/', '\\\\\\\\', $arg);

    // Double up double quotes
    $arg = preg_replace('/"/', '""', $arg);

    // Double up percents.
    // $arg = preg_replace('/%/', '%%', $arg);

    // Add surrounding quotes.
    $arg = '"' . $arg . '"';

    return $arg;
  }

  /**
   * Helper function to generate a random string of arbitrary length.
   *
   * Copied from drush_generate_password(), which is otherwise not available here.
   *
   * @param $length
   *   Number of characters the generated string should contain.
   * @return
   *   The generated string.
   */
  public function randomString($length = 10) {
    // This variable contains the list of allowable characters for the
    // password. Note that the number 0 and the letter 'O' have been
    // removed to avoid confusion between the two. The same is true
    // of 'I', 1, and 'l'.
    $allowable_characters = 'abcdefghijkmnopqrstuvwxyzABCDEFGHJKLMNPQRSTUVWXYZ23456789';

    // Zero-based count of characters in the allowable list:
    $len = strlen($allowable_characters) - 1;

    // Declare the password as a blank string.
    $pass = '';

    // Loop the number of times specified by $length.
    for ($i = 0; $i < $length; $i++) {

      // Each iteration, pick a random character from the
      // allowable string and append it to the password:
      $pass .= $allowable_characters[mt_rand(0, $len)];
    }

    return $pass;
  }

  /**
   *    Accessor for the last output.
   *    @return string        Output as text.
   *    @access public
   */
  function getOutput() {
    return implode("\n", $this->_output);
  }

  /**
   *    Accessor for the last output.
   *    @return array         Output as array of lines.
   *    @access public
   */
  function getOutputAsList() {
    return $this->_output;
  }

  /**
   *    Accessor for the last output, decoded from json.
   *    @param key Optionally return only a top level element from the json object.
   *    @return array         Output as php stdClass object.
   *    @access public
   */
  function getOutputFromJSON($key = NULL) {
    $json = json_decode(current($this->_output));
    if (isset($key)) {
      $json = $json->{$key}; // http://stackoverflow.com/questions/2925044/hyphens-in-keys-of-object
    }
    return $json;
  }

  function webroot() {
    return UNISH_SANDBOX . '/web';
  }

  function directory_cache($subdir = '') {
    return getenv('CACHE_PREFIX') . '/' . $subdir;
  }

  function db_url($env) {
    return substr(UNISH_DB_URL, 0, 6) == 'sqlite'  ?  "sqlite://sites/$env/files/unish.sqlite" : UNISH_DB_URL . '/unish_' . $env;
  }

  function db_driver($db_url = UNISH_DB_URL) {
    return parse_url(UNISH_DB_URL, PHP_URL_SCHEME);
  }

  function setUpDrupal($num_sites = 1, $install = FALSE, $version_string = UNISH_DRUPAL_MAJOR_VERSION, $profile = NULL) {
    $sites_subdirs_all = array('dev', 'stage', 'prod', 'retired', 'elderly', 'dead', 'dust');
    $sites_subdirs = array_slice($sites_subdirs_all, 0, $num_sites);
    $root = $this->webroot();
    $major_version = substr($version_string, 0, 1);

    if (!isset($profile)) {
      $profile = $major_version >= 7 ? 'testing' : 'default';
    }
    $db_driver = $this->db_driver(UNISH_DB_URL);

    $cache_keys = array($num_sites, $install ? 'install' : 'noinstall', $version_string, $profile, $db_driver);
    $source = $this->directory_cache('environments') . '/' . implode('-', $cache_keys) . '.tar.gz';
    if (file_exists($source)) {
      $this->log('Cache HIT. Environment: ' . $source, 'verbose');
      $this->drush('archive-restore', array($source), array('destination' => $root, 'overwrite' => NULL));
    }
    else {
      $this->log('Cache MISS. Environment: ' . $source, 'verbose');
      // Build the site(s), install (if needed), then cache.
      foreach ($sites_subdirs as $subdir) {
        $this->fetchInstallDrupal($subdir, $install, $version_string, $profile);
      }
      // Write an empty sites.php if we are on D8+. Needed for multi-site.
      if ($major_version >= 8 && !file_exists($root . '/sites/sites.php')) {
        copy($root . '/sites/example.sites.php', $root . '/sites/sites.php');
      }
      $options = array(
        'destination' => $source,
        'root' => $root,
        'uri' => reset($sites_subdirs),
        'overwrite' => NULL,
      );
      if ($install) {
        $this->drush('archive-dump', array('@sites'), $options);
      }
    }

    // Stash details about each site.
    foreach ($sites_subdirs as $subdir) {
      $this->sites[$subdir] = array(
        'db_url' => $this->db_url($subdir),
      );
      // Make an alias for the site
      $alias_definition = array($subdir => array('root' => $root,  'uri' => $subdir));
      file_put_contents(UNISH_SANDBOX . '/etc/drush/' . $subdir . '.alias.drushrc.php', unish_file_aliases($alias_definition));
    }
    return $this->sites;
  }

  function fetchInstallDrupal($env = 'dev', $install = FALSE, $version_string = UNISH_DRUPAL_MAJOR_VERSION, $profile = NULL) {
    $root = $this->webroot();
    $site = "$root/sites/$env";


    // Validate
    if (substr($version_string, 0, 1) == 6 && $this->db_driver(UNISH_DB_URL) == 'sqlite') {
      $this->markTestSkipped("Drupal 6 does not support SQLite.");
    }

    // Download Drupal if not already present.
    if (!file_exists($root)) {
      $options = array(
        'destination' => UNISH_SANDBOX,
        'drupal-project-rename' => 'web',
        'yes' => NULL,
        'quiet' => NULL,
        'cache' => NULL,
      );
      $this->drush('pm-download', array("drupal-$version_string"), $options);
      // @todo This path is not proper in D8.
      mkdir(UNISH_SANDBOX . '/web/sites/all/drush', 0777, 1);
    }

    // If specified, install Drupal as a multi-site.
    if ($install) {
      $options = array(
        'root' => $root,
        'db-url' => $this->db_url($env),
        'sites-subdir' => $env,
        'yes' => NULL,
        'quiet' => NULL,
      );
      $this->drush('site-install', array($profile), $options);
      // Give us our write perms back.
      chmod($site, 0777);
    }
    else {
      mkdir($site);
      touch("$site/settings.php");
    }
  }

  /**
   * @see drush_drupal_sitewide_directory()
   */
  function drupalSitewideDirectory($major_version = NULL) {
    if (!$major_version) {
      $major_version = UNISH_DRUPAL_MAJOR_VERSION;
    }
    return ($major_version < 8) ? '/sites/all' : '';
  }
}

abstract class Drush_CommandTestCase extends Drush_TestCase {

  // Unix exit codes.
  const EXIT_SUCCESS  = 0;
  const EXIT_ERROR = 1;
  /**
   * An array of Drupal sites that are setup in the drush-sandbox.
   */
  var $sites;

  /**
   * Actually runs the command. Does not trap the error stream output as this
   * need PHP 4.3+.
   *
   * @param string $command
   *   The actual command line to run.
   * @param integer $expected_return
   *   The return code to expect
   * @param array $env
   *   Extra environment variables
   * @return integer
   *   Exit code. Usually self::EXIT_ERROR or self::EXIT_SUCCESS.
   */
  function execute($command, $expected_return = self::EXIT_SUCCESS, $env = array()) {
    $this->_output = FALSE;
    $return = 1;
    $this->log("Executing: $command", 'notice');

    // Apply the environment variables we need for our test
    // to the head of the command
    $prefix = '';
    foreach ($env as $env_name => $env_value) {
      $prefix .= $env_name . '=' . self::escapeshellarg($env_value) . ' ';
    }
    exec($prefix . $command, $this->_output, $return);

    $this->assertEquals($expected_return, $return, 'Unexpected exit code: ' .  $command);
    return $return;
  }

  /**
   * Invoke drush in via execute().
   *
   * @param command
    *   A defined drush command such as 'cron', 'status' or any of the available ones such as 'drush pm'.
    * @param args
    *   Command arguments.
    * @param $options
    *   An associative array containing options.
    * @param $site_specification
    *   A site alias or site specification. Include the '@' at start of a site alias.
    * @param $cd
    *   A directory to change into before executing.
    * @param $expected_return
    *   The expected exit code. Usually self::EXIT_ERROR or self::EXIT_SUCCESS.
    * @param $suffix
    *   Any code to append to the command. For example, redirection like 2>&1.
    * @return integer
    *   An exit code.
    */
  function drush($command, array $args = array(), array $options = array(), $site_specification = NULL, $cd = NULL, $expected_return = self::EXIT_SUCCESS, $suffix = NULL, $env = array()) {
    $global_option_list = array('simulate', 'root', 'uri', 'include', 'config', 'alias-path', 'ssh-options', 'backend');
    $hide_stderr = FALSE;
    // insert "cd ... ; drush"
    $cmd[] = $cd ? sprintf('cd %s &&', self::escapeshellarg($cd)) : NULL;
    $cmd[] = UNISH_DRUSH;

    // insert global options
    foreach ($options as $key => $value) {
      if (in_array($key, $global_option_list)) {
        unset($options[$key]);
        if ($key == 'backend') {
          $hide_stderr = TRUE;
          $value = NULL;
        }
        if (!isset($value)) {
          $cmd[] = "--$key";
        }
        else {
          $cmd[] = "--$key=" . self::escapeshellarg($value);
        }
      }
    }

    if ($level = $this->log_level()) {
      $cmd[] = '--' . $level;
    }
    $cmd[] = "--nocolor";

    // insert site specification and drush command
    $cmd[] = empty($site_specification) ? NULL : self::escapeshellarg($site_specification);
    $cmd[] = $command;

    // insert drush command arguments
    foreach ($args as $arg) {
      $cmd[] = self::escapeshellarg($arg);
    }
    // insert drush command options
    foreach ($options as $key => $value) {
      if (!isset($value)) {
        $cmd[] = "--$key";
      }
      else {
        $cmd[] = "--$key=" . self::escapeshellarg($value);
      }
    }
    $cmd[] = $suffix;
    if ($hide_stderr) {
      $cmd[] = '2>/dev/null';
    }
    $exec = array_filter($cmd, 'strlen'); // Remove NULLs
    // set sendmail_path to 'true' to disable any outgoing emails
    // that tests might cause Drupal to send.
    $php_options = (array_key_exists('PHP_OPTIONS', $env)) ? $env['PHP_OPTIONS'] . " " : "";
    $env['PHP_OPTIONS'] = "${php_options}-d sendmail_path='true'";
    return $this->execute(implode(' ', $exec), $expected_return, $env);
  }

  function drush_major_version() {
    static $major;

    if (!isset($major)) {
      $this->drush('version', array('drush_version'), array('pipe' => NULL));
      $version = $this->getOutput();
      list($major) = explode('.', $version);
    }
    return $major;
  }
}

/**
 * Base class for Drush unit tests
 *
 * Those tests will run in a bootstrapped Drush environment
 *
 * This should be ran in separate processes, which the following
 * annotation should do in 3.6 and above:
 *
 * @runTestsInSeparateProcesses
 */
abstract class Drush_UnitTestCase extends Drush_TestCase {

  /**
   * Minimally bootstrap drush
   *
   * This is equivalent to the level DRUSH_BOOTSTRAP_NONE, as we
   * haven't run drush_bootstrap() yet. To do anything, you'll need to
   * bootstrap to some level using drush_bootstrap().
   *
   * @see drush_bootstrap()
   */
  public static function setUpBeforeClass() {
    parent::setUpBeforeClass();
    require_once(dirname(__FILE__) . '/../includes/bootstrap.inc');
    drush_bootstrap_prepare();
    // Need to set DRUSH_COMMAND so that drush will be called and not phpunit
    define('DRUSH_COMMAND', UNISH_DRUSH);
  }

  public static function tearDownAfterClass() {
    parent::tearDownAfterClass();
    drush_bootstrap_finish();
  }

  function drush_major_version() {
    return DRUSH_MAJOR_VERSION;
  }
}

/**
 * Initialize our environment at the start of each run (i.e. suite).
 */
function unish_init() {
  // Default drupal major version to run tests over.
  $unish_drupal_major = '7';
  if (getenv('UNISH_DRUPAL_MAJOR_VERSION')) {
    $unish_drupal_major = getenv('UNISH_DRUPAL_MAJOR_VERSION');
  }
  elseif (isset($GLOBALS['UNISH_DRUPAL_MAJOR_VERSION'])) {
    $unish_drupal_major = $GLOBALS['UNISH_DRUPAL_MAJOR_VERSION'];
  }
  define('UNISH_DRUPAL_MAJOR_VERSION', $unish_drupal_major);

  // We read from env then globals then default to mysql.
  $unish_db_url = 'mysql://root:@127.0.0.1';
  if (getenv('UNISH_DB_URL')) {
    $unish_db_url = getenv('UNISH_DB_URL');
  }
  elseif (isset($GLOBALS['UNISH_DB_URL'])) {
    $unish_db_url = $GLOBALS['UNISH_DB_URL'];
  }
  define('UNISH_DB_URL', $unish_db_url);

  // UNISH_DRUSH value can come from phpunit.xml or `which drush`.
  if (!defined('UNISH_DRUSH')) {
    // Let the UNISH_DRUSH environment variable override if set.
    $unish_drush = isset($_SERVER['UNISH_DRUSH']) ? $_SERVER['UNISH_DRUSH'] : NULL;
    $unish_drush = isset($GLOBALS['UNISH_DRUSH']) ? $GLOBALS['UNISH_DRUSH'] : $unish_drush;
    if (empty($unish_drush)) {
      $unish_drush = Drush_TestCase::is_windows() ? exec('for %i in (drush) do @echo.   %~$PATH:i') : trim(`which drush`);
    }
    define('UNISH_DRUSH', $unish_drush);
  }

  define('UNISH_TMP', getenv('UNISH_TMP') ? getenv('UNISH_TMP') : (isset($GLOBALS['UNISH_TMP']) ? $GLOBALS['UNISH_TMP'] : sys_get_temp_dir()));
  define('UNISH_SANDBOX', UNISH_TMP . DIRECTORY_SEPARATOR . 'drush-sandbox');

  // Cache dir lives outside the sandbox so that we get persistence across classes.
  define('UNISH_CACHE', UNISH_TMP . DIRECTORY_SEPARATOR . 'drush-cache');
  putenv("CACHE_PREFIX=" . UNISH_CACHE);
  // Wipe at beginning of run.
  if (file_exists(UNISH_CACHE)) {
    // TODO: We no longer clean up cache dir between runs. Much faster, but we
    // we should watch for subtle problems. To manually clean up, delete the
    // UNISH_TMP/drush-cache directory.
    // unish_file_delete_recursive($cache, TRUE);
  }
  else {
    $ret = mkdir(UNISH_CACHE, 0777, TRUE);
  }

  $home = UNISH_SANDBOX . '/home';
  putenv("HOME=$home");
  putenv("HOMEDRIVE=$home");

  putenv('ETC_PREFIX=' . UNISH_SANDBOX);
  putenv('SHARE_PREFIX=' . UNISH_SANDBOX);
  putenv('TEMP=' . UNISH_TMP);

  define('UNISH_USERGROUP', isset($GLOBALS['UNISH_USERGROUP']) ? $GLOBALS['UNISH_USERGROUP'] : NULL);

  define('UNISH_BACKEND_OUTPUT_DELIMITER', 'DRUSH_BACKEND_OUTPUT_START>>>%s<<<DRUSH_BACKEND_OUTPUT_END');
}

/**
 * Deletes the specified file or directory and everything inside it.
 *
 * Usually respects read-only files and folders. To do a forced delete use
 * drush_delete_tmp_dir() or set the parameter $forced.
 *
 * This is essentially a copy of drush_delete_dir().
 *
 * @todo This sort of duplication isn't very DRY. This is bound to get out of
 *   sync with drush_delete_dir(), as in fact it already has before.
 *
 * @param string $dir
 *   The file or directory to delete.
 * @param bool $force
 *   Whether or not to try everything possible to delete the directory, even if
 *   it's read-only. Defaults to FALSE.
 * @param bool $follow_symlinks
 *   Whether or not to delete symlinked files. Defaults to FALSE--simply
 *   unlinking symbolic links.
 *
 * @return bool
 *   FALSE on failure, TRUE if everything was deleted.
 *
 * @see drush_delete_dir()
 */
function unish_file_delete_recursive($dir, $force = FALSE, $follow_symlinks = FALSE) {
  // Do not delete symlinked files, only unlink symbolic links
  if (is_link($dir) && !$follow_symlinks) {
    return unlink($dir);
  }
  // Allow to delete symlinks even if the target doesn't exist.
  if (!is_link($dir) && !file_exists($dir)) {
    return TRUE;
  }
  if (!is_dir($dir)) {
    if ($force) {
      // Force deletion of items with readonly flag.
      @chmod($dir, 0777);
    }
    return unlink($dir);
  }
  if (unish_delete_dir_contents($dir, $force) === FALSE) {
    return FALSE;
  }
  if ($force) {
    // Force deletion of items with readonly flag.
    @chmod($dir, 0777);
  }
  return rmdir($dir);
}

/**
 * Deletes the contents of a directory.
 *
 * This is essentially a copy of drush_delete_dir_contents().
 *
 * @param string $dir
 *   The directory to delete.
 * @param bool $force
 *   Whether or not to try everything possible to delete the contents, even if
 *   they're read-only. Defaults to FALSE.
 *
 * @return bool
 *   FALSE on failure, TRUE if everything was deleted.
 *
 * @see drush_delete_dir_contents()
 */
function unish_delete_dir_contents($dir, $force = FALSE) {
  $scandir = @scandir($dir);
  if (!is_array($scandir)) {
    return FALSE;
  }

  foreach ($scandir as $item) {
    if ($item == '.' || $item == '..') {
      continue;
    }
    if ($force) {
      @chmod($dir, 0777);
    }
    if (!unish_file_delete_recursive($dir . '/' . $item, $force)) {
      return FALSE;
    }
  }
  return TRUE;
}

/**
 * A slightly less functional copy of drush_backend_parse_output().
 */
function parse_backend_output($string) {
  $regex = sprintf(UNISH_BACKEND_OUTPUT_DELIMITER, '(.*)');
  preg_match("/$regex/s", $string, $match);
  if ($match[1]) {
    // we have our JSON encoded string
    $output = $match[1];
    // remove the match we just made and any non printing characters
    $string = trim(str_replace(sprintf(UNISH_BACKEND_OUTPUT_DELIMITER, $match[1]), '', $string));
  }

  if ($output) {
    $data = json_decode($output, TRUE);
    if (is_array($data)) {
      return $data;
    }
  }
  return $string;
}

/**
 * Prepare the contents of an aliases file.
 */
function unish_file_aliases($aliases) {
  foreach ($aliases as $name => $alias) {
    $records[] = sprintf('$aliases[\'%s\'] = %s;', $name, var_export($alias, TRUE));
  }
  $contents = "<?php\n\n" . implode("\n\n", $records);
  return $contents;
}

 // This code is in global scope.
 // TODO: I would rather this code at top of file, but I get Fatal error: Class 'Drush_TestCase' not found
 unish_init();
