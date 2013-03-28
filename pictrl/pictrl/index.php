<?php
/* pictrl/index.php -- main script for "pictrl" ("Picture Control") app
David Gould, 2013
*/


//CONSTANT DEFINITIONS


require_once('pictrl.inc');

?>
<html>
<title>pictrl</title>
<body bgcolor="ffffff">
<h1><center><font color="2222bb">pi</font><font color="22bbbb">c</font><font color="22bb22">trl</font></center></h1>
<hr/>

<?php

$g_user = $_SERVER['PHP_AUTH_USER'];
print("<h2>User: $g_user</h2>\n\n");
if(!$g_user) { Error('user not logged in'); }

if(($g_user == 'david') && key_exists('phpinfo', $_REQUEST) && $_REQUEST['phpinfo'])
{
	print(phpinfo());
	exit;
}

if(!(file_exists($g_user) && is_dir($g_user)))
{
	mkdir($g_user, 0775);
}
chdir($g_user);



$needSelect = true;
$g_PicKey = GetParam('pic');
$g_Mode = GetParam('mode');
if($g_PicKey && $g_Mode && ($g_Mode == 'edit'))
{
	$needSelect = false;
	include('manage_image.inc');
}

if($needSelect)
{
	include('select_image.inc');
}



?>
</body>
</html>
<?php
exit;


