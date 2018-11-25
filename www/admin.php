<html>
    <head>
        <title>Raspberry PI stuff</title>
        <meta name="viewport" content="initial-scale=1.0, maximum-scale=3.0, width=device-width" />
        <link rel="stylesheet" type="text/css" href="style.css"></link>
        <script type="text/javascript" src="jquery-3.3.1.min.js"></script>
        <script type="text/javascript" src="admin.js"></script>
    </head>
    <body>
        <div class="memory">MemFree: <span id="Memory"></span></div>
        <div class="cpuload">Load: <span id="LoadAvg1">?</span> <span id="LoadAvg5">?</span> <span id="LoadAvg15">?</span></div>
        <div class="cpuperc">CPU: <span id="CpuUsage">?</span>%</div>
        <div class="uptime">UpTime: <span id="UpTime">?</span></div>
        <div class="temperature">Temp: <span id="CpuTemp">?</span>&#176;C</div>
<?php if ($_COOKIE["spelleider"] == "spelleider"): ?>
        <div class="volume">Vol: <span class="button" id="VolDown" volchange="-5">-</span> <span id="Volume">?</span>% <span class="button" id="VolUp" volchange="+5">+</span></div>
	<div class="link"><a href="tweak.php">Tweak settings</a></div>
<?php else: ?>
        <div class="volume">Vol: <span id="Volume">?</span>%</div>
<?php endif; ?>
	<div class="profile">Time/cpu(conns): <span id="Profile_0">?</div>
	<div class="profile">Time/cpu(read): <span id="Profile_1">?</div>
	<div class="profile">Time/cpu(game): <span id="Profile_2">?</div>
	<div class="profile">Time/cpu(write): <span id="Profile_3">?</div>
	<div class="profile">Time/cpu(leds): <span id="Profile_4">?</div>
	<div class="profile">Time/cpu(audio): <span id="Profile_5">?</div>
	<div class="profile">Time/cpu(sleep): <span id="Profile_6">?</div>
    </body>
</html>
