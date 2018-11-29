<html>
    <head>
        <title>Connector Module</title>
        <meta name="viewport" content="maximum-scale=3.0, width=320px" />
        <link rel="stylesheet" type="text/css" href="style.css"></link>
        <script type="text/javascript" src="jquery-3.3.1.min.js"></script>
        <script type="text/javascript" src="main.js"></script>
    </head>
    <body>
        <table id="connectors" spelleider="<?= ($_COOKIE["spelleider"] == "spelleider") ? "true" : "false" ?>"></table>
<?php if ($_COOKIE["spelleider"] == "spelleider"): ?>
        <a href="admin.php">Admin page</a> <a href="sl-logout.php">Logout</a>
	<table class="break">
	    <tr>
		<td class="button" colspan="2" myval="0.0">00</td>
		<td class="button" colspan="2" myval="0.2">20</td>
		<td class="button" colspan="2" myval="0.4">40</td>
		<td class="button" colspan="2" myval="0.6">60</td>
		<td class="button" colspan="2" myval="0.8">80</td>
		<td class="button" colspan="2" myval="1.0">100</td>
	    </tr>
	    <tr>
	    	<td class="spacer"></td>
		<td class="button" colspan="2" myval="0.1">10</td>
		<td class="button" colspan="2" myval="0.3">30</td>
		<td class="button" colspan="2" myval="0.5">50</td>
		<td class="button" colspan="2" myval="0.7">70</td>
		<td class="button" colspan="2" myval="0.9">90</td>
	    	<td class="spacer"></td>
	    </tr> 
	</table>
<?php endif; ?>
    </body>
</html>
