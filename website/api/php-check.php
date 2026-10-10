<?php
header('Content-Type: text/plain');
echo "PHP is running\n";
echo "PHP version: " . PHP_VERSION . "\n";
echo "PDO available: " . (class_exists('PDO') ? 'yes' : 'no') . "\n";
echo "MySQL driver: " . (extension_loaded('pdo_mysql') ? 'yes' : 'no') . "\n";

