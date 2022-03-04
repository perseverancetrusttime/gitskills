#!/bin/bash
cp pre-commit .git/hooks/pre-commit
chmod 777 .git/hooks/pre-commit
echo 'protobuf预检初始化完成'
exit 0
