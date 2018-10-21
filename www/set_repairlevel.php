<?php
$repairlevel = floatval($_POST['repairlevel']);
if ($repairlevel >= 0.0 && $repairlevel <= 1.0) {
    echo "Setting repairlevel to $repairlevel\n";
    $res = file_put_contents('cmd/repair.txt', $repairlevel, LOCK_EX);
    if ((!$res) || ($res < 0)) {
        print_r(error_get_last());
    } else {
        echo "Set repairlevel to $repairlevel : $res\n";
    }
} else {
    echo "Failed to set repairlevel ($repairlevel)";
}
?>
