<?php
header('Content-Type: application/json');
function parse_meminfo($meminfo)
{
    return preg_replace_callback('/^\s*(\d+)\s*(?:([kmgt]?)b?)?\s*$/i', function($m) {
        switch(strtolower($m[2])) {
        case 't': $m[1] *= 1024;
        case 'g': $m[1] *= 1024;
        case 'm': $m[1] *= 1024;
        case 'k': $m[1] *= 1024;
        }
        return $m[1];
    }, $meminfo);
}
$sysinfo = array();
foreach (explode("\n",file_get_contents('/proc/meminfo')) as $memline) {
    list($key, $val) = explode(":", $memline);
    $key = str_replace(")","",str_replace("(","_",$key));
    if ($key != "") {
        $sysinfo[$key] = parse_meminfo(trim($val));
    }
}
$sysinfo["CpuTemp"] = intval(file_get_contents('/sys/class/thermal/thermal_zone0/temp'))/1000;
$sysinfo["Memory"] = sprintf("%.2f/%.2f GB", $sysinfo["MemAvailable"]/(1024*1024),$sysinfo["MemTotal"]/(1024*1024));
echo json_encode(array("sysinfo"=>$sysinfo));
?>
