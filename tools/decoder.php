<?php

  $config = '../asb_proto.h';

  //Read config
  $asb_def = asbGetDefs($config);


  while(1) {
    $line = trim(fgets(STDIN));
    if($line == '') continue;

    $pkg = asbPkgDecode($line);

    if($pkg) {
      echo '---'.PHP_EOL;
      echo strftime("%Y-%m-%d %H:%M:%S").PHP_EOL;
      echo "Packet type: ".$asb_def['asb_types'][$pkg->meta->type].' (0x'.sprintf('%02X', $pkg->meta->type).')'.PHP_EOL;
      echo "Target:      0x".sprintf('%04X', $pkg->meta->target).PHP_EOL;
      echo "Source:      0x".sprintf('%03X', $pkg->meta->source).PHP_EOL;
      echo "Port:        0x".sprintf('%02X', $pkg->meta->port).PHP_EOL;
      echo "Length:      0x".sprintf('%02X', $pkg->len).PHP_EOL;

      for($i=0; $i<$pkg->len; $i++) {
        echo '  '.$i.' => 0x'.sprintf('%02X', $pkg->data[$i]);
        if($i==0 && isset($asb_def['asb_cmd'][$pkg->data[$i]])) echo ' ('.$asb_def['asb_cmd'][$pkg->data[$i]].')';
        echo PHP_EOL;
      }

      echo asbPkgDecodeCmd($pkg->data).PHP_EOL;
      echo '---'.PHP_EOL;
    }else{
      echo 'Not detected: '.$line.PHP_EOL;
      echo '---'.PHP_EOL;
    }
  }


  function asbGetDefs($config) {
    $config = file_get_contents($config);

    $out = array();

    $out['asb_types'] = array();
    preg_match_all('/#define (ASB_PKGTYPE_\w+)\s+0x([0-9A-F]+)/', $config, $match);
    for($i=0; $i<count($match[0]); $i++) {
      $out['asb_types'][hexdec($match[2][$i])] = $match[1][$i];
    }

    $out['asb_cmd'] = array();
    preg_match_all('/#define (ASB_CMD_\w+)\s+0x([0-9A-F]+)(.*)/', $config, $match);
    for($i=0; $i<count($match[0]); $i++) {
      $out['asb_cmd'][hexdec($match[2][$i])] = $match[1][$i];
      if(strlen($match[3][$i]) > 3) $out['asb_cmd'][hexdec($match[2][$i])] .= ' ('.substr($match[3][$i], 3).')';
    }

    return $out;
  }

  function asbPkgDecode($line) {
    if(preg_match('/\\x{01}([0-9A-F]*)\\x{1f}([0-9A-F]*)\\x{1f}([0-9A-F]*)\\x{1f}([0-9A-F]*)\\x{1f}([0-9A-F]*)\\x{02}((([0-9A-F]*)\\x{1f})*)\\x{04}/', $line, $match)) {

      $pkg = new asbPacket;
      $pkg->meta = new asbMeta;

      $pkg->meta->type   = hexdec($match[1]);
      $pkg->meta->target = hexdec($match[2]);
      $pkg->meta->source = hexdec($match[3]);
      $pkg->meta->port   = hexdec($match[4]);
      $pkg->len          = hexdec($match[5]);

      if($pkg->len > 0) {
        preg_match_all('/([0-9A-F]*)\\x{1f}/', $match[6], $data);
        for($i=0; $i<$pkg->len; $i++) {
          $pkg->data[] = hexdec($data[1][$i]);
        }
      }

      return $pkg;
    }else{
      return false;
    }
  }

  class asbMeta{
    /**
	   * Message type
	   *  0 -> Broadcast
	   *  1 -> Multicast
	   *  2 -> Unicast
	   */
	  public $type = 0x00;

	  /**
	   * Port
	   * 0x00 - 0x1F
	   * Only used in Unicast Mode, otherwise -1
	   */
	  public $port = -1;

	  /**
	   * Target address
	   * Unicast:             0x0001 - 0x07FF
	   * Milticast/Broadcast: 0x0001 - 0xFFFF
	   * 0x0000 = invalid packet
	   */
	  public $target = 0x0000;

	  /**
	   * Source address
	   * 0x0001 - 0x07FF
	   * 0x0000 = invalid packet
	   */
	  public $source = 0x0000;
  }

  class asbPacket{
    /**
		 * @see asbMeta
		 */
		public $meta;
		/**
		 * length in bytes, 0-8. -1 indicates invalid packet
		 */
		public $len = -1;
		/**
		 * payload
		 */
		public $data = array();
  }

  function asbPkgDecodeCmd($data) {
    if(count($data) == 0) return;

    switch($data[0]) {
      case 0x21:
        return 'The sending node has just booted';
      case 0x40:
        return 'The sending node requested the current state of this group';
      case 0x50:
        return '0-bit-message';
      case 0x51:
        return '1-bit-message, state is '.$data[1];
      case 0x52:
        return 'percental Message, state is '.$data[1].'%';
      case 0x70:
        return 'PING request';
      case 0x71:
        return 'PONG (PING response)';
      case 0x80:
        return 'Request to read configuration register 0x'.sprintf('%02X', asbPkgDecodeArrToUnsignedInt($data[1], $data[2]));
      case 0x81:
        return 'Request to write configuration register 0x'.sprintf('%02X', asbPkgDecodeArrToUnsignedInt($data[1], $data[2])).' with value 0x'.sprintf('%02X', $data[3]<<8);
      case 0x82:
        return 'Request to activate configuration register 0x'.sprintf('%02X', asbPkgDecodeArrToUnsignedInt($data[1], $data[2]));
      case 0x85:
        return 'Request to change node-ID to 0x'.sprintf('%02X', asbPkgDecodeArrToUnsignedInt($data[1], $data[2]));
      case 0xA0:
        return 'Temperature is '.(asbPkgDecodeArrToSignedInt($data[1], $data[2])/10).'°C';
      case 0xA1:
        return 'Humidity is '.(asbPkgDecodeArrToUnsignedInt($data[1], $data[2])/10).'%RH';
      case 0xA2:
        return 'Pressure is '.(asbPkgDecodeArrToUnsignedInt($data[1], $data[2])/10).'hPa';
      case 0xA5:
        return 'LUX is '.asbPkgDecodeArrToUnsignedLong($data[1], $data[2], $data[3], $data[4]);
      case 0xA6:
        return 'UV-Index is '.(asbPkgDecodeArrToUnsignedInt($data[1], $data[2])/10);
      case 0xA7:
        return 'IR is '.asbPkgDecodeArrToUnsignedLong($data[1], $data[2], $data[3], $data[4]);
      //case 0xB0:
      //case 0xB1:
      case 0xC0:
        return 'Voltage is '.(asbPkgDecodeArrToUnsignedInt($data[1], $data[2])/10).'V';
      case 0xC1:
        return 'Ampere is '.(asbPkgDecodeArrToUnsignedInt($data[1], $data[2])/10).'A';
      case 0xC2:
        return 'Power is '.(asbPkgDecodeArrToUnsignedInt($data[1], $data[2])/10).'VA';
      case 0xD0:
        return 'percental sensor is '.$data[1].'%';
      case 0xD1:
        return 'permille sensor is '.asbPkgDecodeArrToUnsignedInt($data[1], $data[2]).'‰';
      case 0xD2:
        return 'parts per million sensor is '.asbPkgDecodeArrToUnsignedInt($data[1], $data[2]);
      case 0xD5:
        return 'x per year sensor is '.asbPkgDecodeArrToUnsignedInt($data[1], $data[2]);
      case 0xD6:
        return 'x per month sensor is '.asbPkgDecodeArrToUnsignedInt($data[1], $data[2]);
      case 0xD7:
        return 'x per day sensor is '.asbPkgDecodeArrToUnsignedInt($data[1], $data[2]);
      case 0xD8:
        return 'x per hour sensor is '.asbPkgDecodeArrToUnsignedInt($data[1], $data[2]);
      case 0xD9:
        return 'x per minute sensor is '.asbPkgDecodeArrToUnsignedInt($data[1], $data[2]);
      case 0xDA:
        return 'x per second sensor is '.asbPkgDecodeArrToUnsignedInt($data[1], $data[2]);
      default:
        return;
    }
  }

  function asbPkgDecodeArrToUnsignedInt($a1, $a2) {
    return (($a1<<8) & $a2);
  }
  function asbPkgDecodeArrToSignedInt($a1, $a2) {
    $int = asbPkgDecodeArrToUnsignedInt($a1, $a2);
    $hex = sprintf('%04X', $int);
    $int = hexdecs($hex);
    return $int;
  }
  function asbPkgDecodeArrToUnsignedLong($a1, $a2, $a3, $a4) {
    return (($a1<<24) & ($a2<<16) & ($a3<<8) & $a4);
  }
  function asbPkgDecodeArrToSignedLong($a1, $a2, $a3, $a4) {
    $int = asbPkgDecodeArrToUnsignedLong($a1, $a2, $a3, $a4);
    $hex = sprintf('%08X', $int);
    $int = hexdecs($hex);
    return $int;
  }

  //http://php.net/manual/de/function.hexdec.php#97172
  function hexdecs($hex){
    // ignore non hex characters
    $hex = preg_replace('/[^0-9A-Fa-f]/', '', $hex);

    // converted decimal value:
    $dec = hexdec($hex);

    // maximum decimal value based on length of hex + 1:
    //   number of bits in hex number is 8 bits for each 2 hex -> max = 2^n
    //   use 'pow(2,n)' since '1 << n' is only for integers and therefore limited to integer size.
    $max = pow(2, 4 * (strlen($hex) + (strlen($hex) % 2)));

    // complement = maximum - converted hex:
    $_dec = $max - $dec;

    // if dec value is larger than its complement we have a negative value (first bit is set)
    return $dec > $_dec ? -$_dec : $dec;
  }
?>
