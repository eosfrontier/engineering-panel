<?php
if (!isset($_COOKIE["spelleider"]) || ($_COOKIE["spelleider"] != "spelleider")) {
    echo "Failed to set volume: Unauthorized!";
    exit(1);
}
header('Content-Type: application/json');

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

$newvolume = floatval($_POST['volume']);
if ($newvolume >= 0.0 && $newvolume <= 100.0) {
    $sysinfo["SetVolume"] = $newvolume;
    $sysinfo["Executed"] = "amixer sset Softmaster ${newvolume}%";
    $sysinfo["Result"] = exec("amixer sset Softmaster ${newvolume}%");
}

$sysinfo["Volume"] = getvolume();
echo json_encode(array("sysinfo"=>$sysinfo));
?>
