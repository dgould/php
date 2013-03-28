<?php
/* pictrl/select_image.inc - module to select an image
David Gould, 2013

MODULE INTERFACE:

  Input Variables:
    $g_user -- logged-in username
*/
DebugPushOn();

$mode = GetParam('mode', 'post');
$uploadError = false;
$uploaded = false;
$opError = false;
$opStatus = false;
if($mode == 'manage')
{
	$op = GetParam('op', 'post');
	if($op)
	{
		switch($op)
		{
			case 'Upload':
			{
				if($_FILES['file']['error'] > 0)
				{
					$uploadError = "error code '" . $_FILES["file"]["error"] . "' in file upload";
					LogError($uploadError);
					break;
				}
				else
				{
					$uploadFilename = $_FILES["file"]["name"];
					$uploadType = $_FILES["file"]["type"];
					$uploadTmpName = $_FILES["file"]["tmp_name"];
					
					$info = ImageFilenameInfo($uploadFilename);
					if(!$info)
					{
						$uploadError = "invalid upload file name";
						LogError($uploadError);
						break;
					}
					if($uploadType !== $known_image_types[$info['type']]['mime-type'])
					{
						$uploadError = "invalid upload file type";
						LogError($uploadError);
						break;
					}
					
					if(move_uploaded_file($uploadTmpName, $uploadFilename . '.tmp'))
					{
						PicDelete($uploadFilename);
						rename($uploadFilename . '.tmp', $uploadFilename);
						$uploaded = $uploadFilename;
					}
					else
					{
						$uploadError = "error moving uploaded file '$uploadTmpName' to '$uploadFilename.tmp'";
						LogError($uploadError);
					}
				}
			}
			break;
			
			//logic for these ops is identical, except for whether or not to retain originals.
			case 'Copy':
			case 'Rename':
			{
				$new_name = GetParam('new_name');
				if(!$new_name)
				{
					$opError = "no new name specified";
					LogError($opError);
					break;
				}
				
				$selected = GetParam('selected');
				if(!$selected)
				{
					$opError = "no pic selected";
					LogError($opError);
					break;
				}
				if(!is_array($selected)) { $selected = array($selected); }
				if(count($selected) > 1)
				{
					$opError = "multiple pics selected for $op (" . implode(',', $selected) . ")";
					LogError($opError);
					break;
				}
				$selected = $selected[0];
				
				//loading $selected also validates it
				$pic = PicScan($selected);
				if(!$pic && (count($pic['sizes']) > 0))
				{
					$opError = "no images found for pic '$selected'";
					LogError($opError);
					break;
				}
				
				//validate $new_name
				//Pass $emptyOk=true, so we get an empty pic structure, instead of false,
				//if the name is valid but no matching images exist. But we get false if
				//the name doesn't parse with a valid type suffix.
				//Passing $invalidOk=true is just to silence the error log message in the latter case.
				$new_pic = PicScan($new_name, true, true); 
				if(!$new_pic)
				{
					//No problem, just means user entered name without suffix, so
					//use $selected's suffix
					$new_name = $new_name . '.' . $pic['suffix'];
					//can't fail, since we just appended what we know to be a valid suffix
					$new_pic = PicScan($new_name, true); 
				}
				
				//delete any existing images matching the new name
				if(count($new_pic['sizes']) > 0)
				{
					PicDelete($new_name);
					$new_pic['sizes'] = array();
				}
				
				//rename the selected images
				foreach($pic['sizes'] as $size)
				{
					$oldFilename = ImageMakeFilename($pic['name'], $size, $pic['suffix']);
					$newFilename = ImageMakeFilename($new_pic['name'], $size, $new_pic['suffix']);
					if($new_pic['type'] == $pic['type'])
					{
						if($op == 'Copy')
						{
							copy($oldFilename, $newFilename);
							LogInfo("copied '$oldFilename' to '$newFilename'");
						}
						else
						{
							rename($oldFilename, $newFilename);
							LogInfo("renamed '$oldFilename' to '$newFilename'");
						}
					}
					else
					{
						$oldInfo = ImageFilenameInfo($oldFilename);
						$img = LoadImage($oldInfo);
						if($img)
						{
							$newInfo = ImageFilenameInfo($newFilename);
							SaveImage($img, $newInfo);
							imagedestroy($img);
							LogInfo("transcoded '$oldFilename' to '$newFilename'");
						}
						if($op = 'Rename')
						{
							unlink($oldFilename);
							LogInfo("deleted '$oldFilename'");
						}
					}
				}
				$opStatus = (($op == 'Copy') ? 'Copied' : 'Renamed') . " $selected to $new_name";
			}
			break;
			
			case 'Delete':
			{
				$selected = GetParam('selected');
				if(!$selected)
				{
					$opError = "no pic seleced";
					LogError($opError);
					break;
				}
				if(!is_array($selected)) { $selected = array($selected); }
				
				foreach($selected as $picKey)
				{
					$count = PicDelete($picKey);
					$opStatus = ($opStatus ? ($opStatus . "<br/>\n") : '') .
					            "Deleted $picKey ($count images)";
				}
			}
			break;
			
			default:
			{
				LogError("unknown op '$op'");
			}
		}
	}
	else
	{
		LogError("mode=manage but no op specified");
	}
}

$dir = opendir('.');
if(!$dir) { Error("can't read directory '" . getcwd() . "'"); }
$items = array();
while($item = readdir($dir))
{
	if(in_array($item, array('.', '..'))) { continue; }
	$items[] = $item;
}
closedir($dir);

$pics = array();
foreach($items as $item)
{
	$info = ImageFilenameInfo($item);
	if(!is_array($info)) { continue; }
	LogDebug("scanning image $item");
	
	if(($info['width'] < 0) || ($info['height'] < 0))
	{
		LogInfo("detected un-size-tagged filename '$item' (width=" . $info['width'] . ",height=" . $info['height'] . ")");
		if(!(($info['width'] < 0) && ($info['height'] < 0))) { LogError("$item has inconsistent dimensions width=$width,heing=$height"); }
		$img = LoadImage($info);
		if(!$img) { continue; }
		$width  = imagesx($img);
		$height = imagesy($img);
		imagedestroy($img);
		
		$newFilename = $info['name'] . "." . SizeMakeTag($width, $height) . "." . $info['suffix'];
		LogInfo("renaming '$item' to '$newFilename'");
		if(!rename($item, $newFilename))
		{
			LogError("failed to rename '$item' to '$newFilename'");
			continue;
		}
		
		$info['filename'] = $newFilename;
		$info['width'] = $width;
		$info['height'] = $height;
	}
	
	$picKey = $info['name'] . '.' . $info['suffix'];
	if(!isset($pics[$picKey])) 
	{
		$pic = array();
		$pic['name'] = $info['name'];
		$pic['type'] = $info['type'];
		$pic['suffix'] = $info['suffix'];
		$pic['sizes'] = array();
		$pics[$picKey] = $pic;
	}
	$pics[$picKey]['sizes'][] = SizeMakeTag($info['width'], $info['height']);
}

?>
<form method="post" name="form_upload" enctype="multipart/form-data">
<input type="hidden" name="mode" value="manage" />
<table border="2" width="100%">
<?php
if($uploadError) { print("\t<tr><th bgcolor=\"dd5555\">$uploadError</th></tr>\n"); }
if($uploaded) { print("\t<tr><th bgcolor=\"55dd55\">Uploaded $uploaded</th></tr>\n"); }
?>
	<tr><td align="center">
		<label for="file">Upload an image:</label>
		<input type="file" name="file" id="file" /> 
		<input type="submit" name="op" value="Upload" />
	</td></tr>
</table>
</form>
<hr/>

<form method="post" name="form_manage">
<input type="hidden" name="mode" value="manage" />
<table border="2" width="100%">
<?php
if($opError) { print("<tr><th colspan=\"5\" bgcolor=\"dd5555\">$opError</th></tr>\n"); }
if($opStatus) { print("<tr><th colspan=\"5\" bgcolor=\"55dd55\">$opStatus</th></tr>\n"); }
?>
<tr>
	<td colspan="5" align="center">
		<table><tr>
			<td>New Name: <input type="text" size="32" name="new_name" /></td>
			<td><input type="submit" name="op" value="Rename" /></td>
			<td><input type="submit" name="op" value="Copy" /></td>
			<td><input type="submit" name="op" value="Delete" /></td>
		</tr></table>
	</td>
</tr>

<tr>
	<th>Preview</th>
	<th>Name</th>
	<th>Type</th>
	<th>Action</th>
	<th>Sizes</th>
</tr>

<?php
$picKeys = array_keys($pics);
sort($picKeys);
foreach($picKeys as $picKey)
{
	$pic = $pics[$picKey];
	print("<tr> <!-- picKey='$picKey' -->\n");

	$preview = PicGetPreview($pic);
	print("\t<td><img src=\"$g_user/$preview\" /></td>\n");
	
	print("\t<td valign=\"top\">" . $pic['name'] . "</td>\n");
	print("\t<td valign=\"top\">" . $pic['type'] . "</td>\n");
	
	print("\t<td valign=\"top\">");
	print("\t\t<a href=\"?mode=edit&pic=" . urlencode($picKey) . "\">edit</a><br/>\n");
	print("\t\t<input type=\"checkbox\" name=\"selected[]\" value=\"$picKey\" /> select\n");
	print("\t</td>\n");
	
	print("\t<td valign=\"top\">\n");
	sort($pic['sizes'], SORT_NUMERIC);
	for($i = 0; $i < count($pic['sizes']); $i++)
	{
		$size = $pic['sizes'][$i];
		$filename = ImageMakeFilename($pic['name'], $size, $pic['suffix']);
		$size = '<a href="' . $g_user . '/' . $filename . '">' . $size . '</a>';
		if($i > 0) { $size = '&nbsp;' . $size; }
		print("\t\t$size\n");
	}
	print("\t</td>\n");
	print("</tr>\n");
}
?>

</table>
<?php
DebugPop();