<?php
/* pictrl/manage_image.inc - module to manage a selected image
David Gould, 2013

MODULE INTERFACE:

  Input Variables:
    $g_PicKey
      Identifier for the selected pic. Always set and valid (else we wouldn't be here).

*/
DebugPushOn();

$pic = PicScan($g_PicKey);

/*
Everything this module does is going to post back to this page, with the same pic selected,
except for the "Back to Library" link.
So, wrap it all in a form, with mode=edit and the pic selection pre-supplied as hidden inputs.

*/
?>
<a href="?">Back to Library</a>
<hr/>

<form method="post">
<input type="hidden" name="pic" value="<?php print($g_PicKey); ?>" />
<input type="hidden" name="mode" value="edit" />
<h2>selected pic: <?php print($g_PicKey); ?></h2>
<?php if(DebugIsOn()) { print("\n<!-- pic info:\n"); var_dump($pic); print("-->\n\n"); } ?>

<?php
$opError = false;
$opStatus = false;
$op = GetParam('op', 'post');
if($op)
{
	switch($op)
	{
		case 'Create Size':
		{
			$new_width = GetParam('new_width');
			$new_height = GetParam('new_height');
			$newFile = PicMakeSize($pic, $new_width, $new_height);
			if($newFile)
			{
				$opStatus = "Created $newFile";
			}
			else
			{
				$opError = "MakeSize($new_width, $new_height) failed";
				LogError($opError);
			}
		}
		break;
		
		case 'Rotate':
		{
			$angle = GetParam('angle');
			PicRotate($pic, $angle);
			$opStatus = "rotated pic $g_PicKey";
		}
		break;
		
		case 'Delete':
		{
			$selected = GetParam('selected');
			if(!is_array($selected)) { $selected = array($selected); }
			PicDelete($g_PicKey, $selected);
			$opStatus = "Deleted sizes: " . implode(',', $selected);
		}
		break;
		
		case 'Crop':
		{
			$top = GetParam('top');
			$left = GetParam('left');
			$bottom = GetParam('bottom');
			$right = GetParam('right');
			$opError = "crop operation not implemented";
		}
		break;
		
		case 'Caption':
		{
			$text = GetParam('text');
			$x = GetParam('x');
			$y = GetParam('y');
			$font = GetParam('font');
			$size = GetParam('size');
			$opError = "caption operation not implemented";
		}
		break;
		
		default:
		{
			LogError("unknown op '$op'");
		}
	}
}
$pic = PicScan($g_PicKey);

if($opError) { print("<table border=\"2\" width=\"100%\"><tr><th bgcolor=\"dd5555\">$opError</th></tr></table>\n<hr/>\n\n"); }
if($opStatus) { print("<table border=\"2\" width=\"100%\"><tr><th bgcolor=\"55dd55\">$opStatus</th></tr></table>\n<hr/>\n\n"); }

?>

<table border="2" width="100%">
<tr>
	<td>
		<b>Create a new size</b><br/>
		<label for="new_width">max width:</label>
		<input type="text" size="5" name="new_width" />
		&nbsp; 
		<label for="new_height">max height:</label>
		<input type="text" size="5" name="new_height" />
		&nbsp; 
		<input type="submit" name="op" value="Create Size" />
	</td>
</tr>
</table>

<hr/>

<table border="2" width="100%">
<tr>
	<td>
		<b>Rotate</b><br/>
		<input type="radio" name="angle" id="rotate_cw" value="270" />
		<label for="rotate_cw">90&deg; clockwise</label>
		&nbsp; 
		<input type="radio" name="angle" id="rotate_180" value="180" />
		<label for="rotate_180">180&deg;</label>
		&nbsp; 
		<input type="radio" name="angle" id="rotate_ccw" value="90" />
		<label for="rotate_ccw">90&deg; counter-clockwise</label>
		&nbsp; 
		<input type="submit" name="op" value="Rotate" />
	</td>
</tr>
</table>

<hr/>

<table border="2" width="100%">
<tr>
	<td>
		<b>Delete Size(s)</b><br/>
<?php
$sizes = $pic['sizes'];
sort($sizes, SORT_NUMERIC);
foreach($sizes as $size)
{
	$filename = ImageMakeFilename($pic['name'], $size, $pic['suffix']);
	print("\t\t<input type=\"checkbox\" id=\"delete_$size\" name=\"selected[]\" value=\"$size\" />\n");
	print("\t\t<label for=\"delete_$size\"><a href=\"$g_user/$filename\">$size</a></label>\n");
	print("\t\t&nbsp;\n");
}
?>
		<br/>
		<input type="submit" name="op" value="Delete" />
	</td>
</tr>
</table>



<?php
DebugPop();
?>
</form>
