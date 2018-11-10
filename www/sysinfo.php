<?php
header('Content-Type: application/json');
function parse_meminfo($meminfo)
{
    return intval(preg_replace_callback('/^\s*(\d+)\s*(?:([kmgt]?)b?)?\s*$/i', function($m) {
        switch(strtolower($m[2])) {
        case 't': $m[1] *= 1024;
        case 'g': $m[1] *= 1024;
        case 'm': $m[1] *= 1024;
        case 'k': $m[1] *= 1024;
        }
        return $m[1];
    }, $meminfo));
}
function getvolume()
{
    exec("amixer sget Softmaster", $volout);
    $tot = 0; $cnt = 0;
    foreach ($volout as $volline) {
        if (preg_match('/\[(\d+)%\]/', $volline, $matches)) {
            $tot += intval($matches[1]);
            $cnt++;
        }
    }
    if ($cnt > 0) {
        return $tot/$cnt;
    } else {
        return -1;
    }
}
$sysinfo = array();
foreach (explode("\n",file_get_contents('/proc/meminfo')) as $memline) {
    list($key, $val) = explode(":", $memline);
    $key = str_replace(")","",str_replace("(","_",$key));
    if ($key != "") {
        $sysinfo[$key] = parse_meminfo(trim($val));
    }
}
$sysinfo["Memory"] = sprintf("%.2f/%.2f GB", $sysinfo["MemAvailable"]/(1024*1024),$sysinfo["MemTotal"]/(1024*1024));
$sysinfo["CpuTemp"] = intval(file_get_contents('/sys/class/thermal/thermal_zone0/temp'))/1000;
$sysinfo["Volume"] = getvolume();
if (preg_match('/([\d.]+)\s*([\d.]+)\s*([\d.]+)\s*([\d]+)\/([\d]+)/', file_get_contents('/proc/loadavg'), $matches)) {
    $sysinfo["LoadAvg1"] = floatval($matches[1]);
    $sysinfo["LoadAvg5"] = floatval($matches[2]);
    $sysinfo["LoadAvg15"] = floatval($matches[3]);
    $sysinfo["RunningProcs"] = floatval($matches[4]);
    $sysinfo["TotalProcs"] = floatval($matches[5]);
}
if (preg_match('/([\d.]+)\s*([\d.]+)/', file_get_contents('/proc/uptime'), $matches)) {
    $sysinfo["UpTimeSecs"] = floatval($matches[1]);
    $sysinfo["IdleTimeSecs"] = floatval($matches[2]);
    $sysinfo["UpTime"] = gmdate("H:i:s", $sysinfo["UpTimeSecs"]);
}
echo json_encode(array("sysinfo"=>$sysinfo));
?>
