<?php
require_once('ESL.php');

$esl = new eslConnection('127.0.0.1', '8021', 'ClueCon');
echo $esl->sendRecv("api status");
