<?php
$spelleider = ($_COOKIE["spelleider"] == "spelleider");
$melding = "";
if (isset($_POST["submit"])) {
  $spelleiderwachtwoord = file_get_contents('../spelleiderwachtwoord.txt');
  if ($_POST["password"] == $spelleiderwachtwoord) {
    $spelleider = true;
    $_SESSION["spelleider"] = "spelleider";
  } else {
    $melding = "Incorrect wachtwoord";
  }
}
if ($spelleider) {
  header("Location: /"); die();
}
?>
<html>
    <head>
	<title>Spelleider login</title>
	<meta name="viewport" content="initial-scale=1.0, maximum-scale=1.0, width=device-width" />
	<link rel="stylesheet" type="text/css" href="style.css"></link>
    </head>
    <body>
	<p>Alleen voor spelleiders!</p>
        <form name="input" action="" method="post">
            <label for="password">Spelleider wachtwoord:</label><input type="password" id="password" name="password"><br>
            <button type="submit">Login</button>
        </form>
        <div class="melding"><?= $melding ?></div>
    </body>
</html>
