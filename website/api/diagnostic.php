<?php
ini_set('display_errors', '0');
ini_set('log_errors', '1');
error_reporting(E_ALL);
header('Content-Type: text/plain');
echo "PHP is running\n";
echo "PHP version: " . PHP_VERSION . "\n";
echo "Current directory: " . getcwd() . "\n";
echo "Config file exists: " .
     (is_file(__DIR__ . '/../cwhip-config.php') ? 'yes' : 'no') . "\n";
echo "PDO available: " . (class_exists('PDO') ? 'yes' : 'no') . "\n";
