<?php
/* pictrl/pictrl.inc -- function library for "pictrl" ("Picture Control") app
David Gould, 2013
*/

// CONSTANTS
define('UNIX_DATE_FMT', '%a %b %d %H:%M:%S %Z %Y');

define('PREVIEW_MAX_WIDTH',  96);
define('PREVIEW_MAX_HEIGHT', 96);

$known_image_types = array
(
	'JPEG' => array
	(
		'mime-type' => 'image/jpeg',
		'suffixes' => array('jpg', 'jpeg'),
	),
	'GIF'  => array
	(
		'mime-type' => 'image/gif',
		'suffixes' => array('gif'),
	),
	'PNG'  => array
	(
		'mime-type' => 'image/png',
		'suffixes' => array('png'),
	),
);


// UTILITIES

function Error($msg)
{
	LogError($msg);
	print("\n\n<!-- ERROR: $msg -->\n\n");
	exit;
}

function GetParam($param, $method = false)
{
	DebugPushOff();
	if($method)
	{
		switch($method)
		{
			case 'post':
			{
				$value = isset($_POST[$param]) ? $_POST[$param] : false;
			}
			break;
			
			case 'get':
			{
				$value = isset($_GET[$param]) ? $_POST[$param] : false;
			}
			break;
			
			default:
			{
				LogError("unknown request method '$method'");
				$value = false;
			}
		}
	}
	else
	{
		$value = isset($_REQUEST[$param]) ? $_REQUEST[$param] : false;
	}
	LogDebug("param $param='$value'");
	DebugPop();
	return $value;
}

// DEBUG/LOGGING

$g_DebugStack = array(false);
function DebugIsOn()
{
	global $g_DebugStack;
	return $g_DebugStack[count($g_DebugStack) - 1];
}
function DebugPush($state)
{
	global $g_DebugStack;
	$g_DebugStack[] = $state;
}
function DebugPushOn()
{
	DebugPush(true);
}
function DebugPushOff()
{
	DebugPush(true);
}
function DebugPushCurr()
{
	DebugPush(DebugIsOn());
}
function DebugPop()
{
	global $g_DebugStack;
	array_pop($g_DebugStack);
}

#@$%*&!ing time/date functions all emit "It is not safe to rely on the 
#system's timezone settings.[...]" warning unless date_default_timezone_set()
#is called first. date_default_timezone_get() is able to get the system
#setting, but would still emit the warning once, so call it with '@'.
@date_default_timezone_set(date_default_timezone_get());
function LogMsg($msg, $tag = 'LOG  ', $depth = 1, $fullStack = false)
{
	$backtrace = debug_backtrace();
	$frame = $backtrace[$depth];
	$file = $frame['file'];
	$line = $frame['line'];
	$func = $frame['function'];
	
	$location = "$file:$line";
	if($fullStack)
	{
		for($i = $depth + 1; $i < count($backtrace); $i++)
		{
			$frame = $backtrace[$i];
			$file = $frame['file'];
			$line = $frame['line'];
			$location = "$file:$line=>" . $location;
		}
	}
	
	//uncomment is error_log() doesn't automatically prepend timestamp
	$msg = /*'[' . strftime(UNIX_DATE_FMT, time()) . '] ' .*/ "$tag in $func ($location) $msg";
	error_log($msg);
}
function LogDebug($msg)
{
	if(DebugIsOn())
	{
		LogMsg($msg, 'DEBUG', 1);
	}
}
function LogError($msg)
{
	LogMsg($msg, 'ERROR', 1, true);
}
function LogWarning($msg)
{
	LogMsg($msg, 'WARN ', 1, true);
}
function LogInfo($msg)
{
	LogMsg($msg, 'INFO ', 1);
}
function LogStats($data)
{
	$msg = '';
	$keys = array_keys($data);
	sort($keys);
	foreach($keys as $key)
	{
		$val = $data[$key];
		if($msg !== '') { $msg .= '&'; }
		$msg .= urlencode($key) . '=' . urlencode($val);
	}
	LogMsg($msg, 'STATS', 2);
}
function DecodeStatsLine($line)
{
	$line = trim($line);
	if(!preg_match('/^\[(.+?)\] \[error\] \[client (.+?)\] STATS in .+? \(.+?\) (.+)(, referer: (.*))?$/', $line, $matches)) { return false; }
	$timestamp = $matches[1];
	$client_ip = $matches[2];
	$dataStr   = $matches[3];
	$referer   = $matches[5];
	
	$info = array();
	$info['timestamp'] = $timestamp;
	$info['client_ip'] = $client_ip;
	$info['referer']   = $referer;
	$info['data']      = array();
	
	$dataParts = explode('&', $dataStr);
	foreach($dataParts as $part)
	{
		if(!preg_match('/^(.+?)=(.*)$/', $part, $matches))
		{
			$key = urldecode($matches[1]);
			$val = urldecode($matches[2]);
			$info['data'][$key] = $val;
		}
	}
	
	return $info;
}

// APPLICATION LOGIC

function ImageMakeFilename($name, $size, $suffix)
{
	return $name . '.' . $size . '.' . $suffix;
}

function SizeMakeTag($width, $height)
{
	return $width . 'x' . $height;
}

function SuffixToType($fileSuffix)
{
	static $memo = array();
	if(isset($memo[$fileSuffix])) { return $memo[$fileSuffix]; }
	
	$imgSuffix = strtolower($fileSuffix);
	global $known_image_types;
	foreach($known_image_types as $type => $typeInfo)
	{
		$suffixes = $typeInfo['suffixes'];
		foreach($suffixes as $suffix)
		{
			if($suffix == $imgSuffix)
			{
				$memo[$fileSuffix] = $type;
				return $type;
			}
		}
	}
	LogError("unrecognized filename suffix '$fileSuffix'");
	return false;
}

function ImageFilenameInfo($filename)
{
	static $memo = array();
	if(isset($memo[$filename]))
	{
		return $memo[$filename];
	}
	
	$info = array('filename' => $filename);
	
	if(!preg_match('/^(.+)\.(\d+)x(\d+)\.([a-zA-Z0-9]+)$/', $filename, $matches)) 
	{
		if(preg_match('/^(.+)\.([a-zA-Z0-9]+)$/', $filename, $matches))
		{
			$matches[4] = $matches[2];
			$matches[2] = -1;
			$matches[3] = -1;
		}
		else
		{
			$memo[$filename] = false;
			return false;
		}
	}
	$info['name']   = $matches[1];
	$info['width']  = $matches[2];
	$info['height'] = $matches[3];
	$info['suffix'] = $matches[4];

	$imgSuffix = strtolower($info['suffix']);
	$type = SuffixToType($imgSuffix);
	if($type)
	{
		$info['type'] = $type;
	}
	else
	{
		$info = false;
	}
	$memo[$filename] = $info;
	return $info;
}

function LoadImage($info)
{
	$filename = $info['filename'];
	$img = false;
	switch($info['type'])
	{
		case 'JPEG':
		{
			$img = imagecreatefromjpeg($filename);
		}
		break;
		
		case 'GIF':
		{
			$img = imagecreatefromgif($filename);
		}
		break;
		
		case 'PNG':
		{
			$img = imagecreatefrompng($filename);
		}
		break;
		
		default:
		{
			LogError("$filename - unknown image type '" . $info['type'] . "'");
		}
	}
	
	if(!$img) { LogError("failed to load image $filename"); }
	return $img;
}
function SaveImage($img, $info)
{
	$filename = $info['filename'];
	switch($info['type'])
	{
		case 'JPEG':
		{
			return imagejpeg($img, $filename);
		}
		break;
		
		case 'GIF':
		{
			return imagegif($img, $filename);
		}
		break;
		
		case 'PNG':
		{
			return imagepng($img, $filename);
		}
		break;
	}
	LogError("$filename - unknown image type '" . $info['type'] . "'");
	return false;
}

function PicGetPreview(&$pic)
{
	foreach($pic['sizes'] as $size)
	{
		if(!preg_match('/^(\d+)x(\d+)$/', $size, $matches)) { LogError("invalid size '$size'"); continue; }
		$width = $matches[1];
		$height = $matches[2];
		if((($width == PREVIEW_MAX_WIDTH) && ($height <= PREVIEW_MAX_HEIGHT)) ||
		   (($width <= PREVIEW_MAX_WIDTH) && ($height == PREVIEW_MAX_HEIGHT)))
		{
			return ImageMakeFilename($pic['name'], $size, $pic['suffix']);
		}
	}
	
	return PicMakePreview($pic);
}

function PicGetBiggest($pic)
{
	$maxWidth = 0;
	$maxSize = false;
	foreach($pic['sizes'] as $size)
	{
		if(!preg_match('/^(\d+)x(\d+)$/', $size, $matches)) { LogError("invalid size '$size'"); continue; }
		$width = $matches[1];
		if($width > $maxWidth) { $maxWidth = $width; $maxSize = $size; }
	}
	return ImageMakeFilename($pic['name'], $maxSize, $pic['suffix']);
}

function PicMakePreview(&$pic)
{
	return PicMakeSize(&$pic, PREVIEW_MAX_WIDTH, PREVIEW_MAX_HEIGHT);
}

function PicMakeSize(&$pic, $maxWidth, $maxHeight)
{
	$biggestFilename = PicGetBiggest($pic);
	$biggestInfo = ImageFilenameInfo($biggestFilename);
	$biggestImg = LoadImage($biggestInfo);
	
	$newSize = '';
	$xFactor = $biggestInfo['width'] / $maxWidth;
	$yFactor = $biggestInfo['height'] / $maxHeight;
	$factor = max($xFactor, $yFactor);
	$newWidth = floor($biggestInfo['width'] / $factor);
	$newHeight = floor($biggestInfo['height'] / $factor);
	$newSize = SizeMakeTag($newWidth, $newHeight);
	$newFilename = ImageMakeFilename($pic['name'], $newSize, $pic['suffix']);
	if(in_array($newSize, $pic['sizes'])) { return $newFilename; }
	
	$newImg = imagecreatetruecolor($newWidth, $newHeight);
	imagecopyresampled($newImg, $biggestImg,  0, 0,  0, 0,  $newWidth, $newHeight,  $biggestInfo['width'], $biggestInfo['height']);
	imageDestroy($biggestImg);
	
	$newInfo = $biggestInfo;
	$newInfo['width'] = $newWidth;
	$newInfo['height'] = $newHeight;
	$newInfo['filename'] = $newFilename;
	
	$success = SaveImage($newImg, $newInfo);
	imageDestroy($newImg);
	
	if($success)
	{
		$pic['sizes'][] = $newSize;
		return $newFilename;
	}
	return false;
}

function PicScan($picKey, $emptyOk = false, $invalidOk = false)
{
	if(!preg_match('/^(.+)\.([a-zA-Z0-9]+?)$/', $picKey, $matches))
	{
		if(!$invalidOk) { LogError("invalid picKey '$picKey'"); }
		return false;
	}
	$name = $matches[1];
	$suffix = $matches[2];
	$type = SuffixToType($suffix);
	if(!$type) { return false; }
	
	$pic = array
	(
		'name'   => $name,
		'suffix' => $suffix,
		'type'   => $type,
		'sizes'  => array()
	);
	
	$dir = opendir('.');
	if(!$dir) { Error("can't read directory '" . getcwd() . "'"); }
	while($item = readdir($dir))
	{
		if(in_array($item, array('.', '..')) || preg_match('/\.tmp$/', $item)) { continue; }
		$info = ImageFilenameInfo($item);
		if($info && ($info['name'] == $name) && ($info['suffix'] == $suffix))
		{
			$pic['sizes'][] = SizeMakeTag($info['width'], $info['height']);
		}
	}
	closedir($dir);
	
	return ($emptyOk || (count($pic['sizes']) > 0)) ? $pic : false;
}

/* Delete the specified-size images matching $picKey, 
   or ALL images, if no sizes specified.
*/
function PicDelete($picKey, $sizes = false)
{
	$pic = PicScan($picKey);
	$count = 0;
	if(!$pic) { return $count; }
	foreach($pic['sizes'] as $size)
	{
		if(is_array($sizes))
		{
			if(!in_array($size, $sizes))
			{
				continue;
			}
		}
		$filename = ImageMakeFilename($pic['name'], $size, $pic['suffix']);
		if(file_exists($filename))
		{
			if(unlink($filename))
			{
				LogInfo("deleted '$filename'");
				$count++;
			}
			else
			{
				LogError("failed to delete '$filename'");
			}
		}
	}
	return $count;
}

function PicRotate(&$pic, $angle)
{
	$oldSizes = $pic['sizes'];
	$newSizes = array();
	$name = $pic['name'];
	$suffix = $pic['suffix'];
	
	foreach($oldSizes as $oldSize)
	{
		$oldFilename = ImageMakeFilename($name, $oldSize, $suffix);
		$oldInfo = ImageFilenameInfo($oldFilename);
		$oldImg = LoadImage($oldInfo);
		$color = imagecolorallocate($oldImg, 0, 0, 0);
		$newImg = imagerotate($oldImg, $angle, $color);
		imagedestroy($oldImg);
		$newSize = SizeMakeTag(imagesx($newImg), imagesy($newImg));
		$newFilename = ImageMakeFilename($name, $newSize, $suffix);
		$newInfo = ImageFilenameInfo($newFilename);
		SaveImage($newImg, $newInfo);
		imagedestroy($newImg);
		if($newSize !== $oldSize)
		{
			unlink($oldFilename);
		}
		$newSizes[] = $newSize;
	}
	$pic['sizes'] = $newSizes;
}


