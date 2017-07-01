<?php

  if($argc < 4) {
    echo 'Usage: '.$argv[0].' [TYPE] [TARGET] [SOURCE] [PORT] {[DATA1] [DATA2] ...} > /dev/ttyUSBx'.PHP_EOL;
    echo 'All values as hex without prefix.'.PHP_EOL;
    echo PHP_EOL;
    echo 'TYPE:   0 to 2'.PHP_EOL;
    echo '        0 -> Boradcast'.PHP_EOL;
    echo '        1 -> Multicast'.PHP_EOL;
    echo '        2 -> Unicast'.PHP_EOL;
    echo PHP_EOL;
    echo 'TARGET: 1 to 7FF for Unicast'.PHP_EOL;
    echo '        1 to FFFF for Multcast/Broadcast'.PHP_EOL;
    echo PHP_EOL;
    echo 'SOURCE: 0 to 7FF'.PHP_EOL;
    echo PHP_EOL;
    echo 'PORT:   0 to 1F for Unicast'.PHP_EOL;
    echo '        FF Multicast/Broadcast'.PHP_EOL;
    echo PHP_EOL;
    echo 'DATA:   Up to 8 Bytes'.PHP_EOL;
    exit(1);
  }

  $data = array();
  for($i=5; $i<$argc; $i++) $data[] = hexdec($argv[$i]);

  echo asbPkgEncode(hexdec($argv[1]), hexdec($argv[2]), hexdec($argv[3]), hexdec($argv[4]), $data);

  function asbPkgEncode($type, $target, $source, $port=-1, $data) {
    $out  = chr(0x01);
    $out .= dechex($type);
    $out .= chr(0x1F);
    $out .= dechex($target);
    $out .= chr(0x1F);
    $out .= dechex($source);
    $out .= chr(0x1F);
    if($port < 0) {
      $out .= 'ff';
    }else{
      $out .= dechex($port);
    }
    $out .= chr(0x1F);
    $out .= dechex(count($data));
    $out .= chr(0x02);
    for($i=0; $i<count($data); $i++) {
      $out .= dechex($data[$i]);
      $out .= chr(0x1F);
    }
    $out .= chr(0x04);
    $out .= "\r\n";
    return strtoupper($out);
  }

?>
