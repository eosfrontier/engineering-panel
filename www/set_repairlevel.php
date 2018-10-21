<?php
$repairlevel = floatval($_POST['repairlevel']);
if ($repairlevel >= 0.0 && $repairlevel <= 1.0) {
    file_put_contents('cmd/repair.txt', $repairlevel);
}
?>
