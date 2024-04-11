#!/bin/sh

SQLITE3_PATH=/usr/bin/sqlite3

# DB_PATH=/opt/data

DB_PATH=/tmp
DB_UPL_DATA=$DB_PATH/upl_data.db
DB_UPL_STATE=$DB_PATH/upl_state.db
DB_UPL_CONFIG=$DB_PATH/upl_config.db

mkdir -p $DB_PATH

if [ -f $DB_UPL_DATA ]; then
	echo "$DB_UPL_DATA already existed!!"
	exit 1
else
	echo "Create database ${DB_UPL_DATA}."
	touch $DB_UPL_DATA
fi

if [ -f $DB_UPL_CONFIG ]; then
	echo "$DB_UPL_CONFIG already existed!!"
	exit 1
else
	echo "Create database ${DB_UPL_CONFIG}."
	touch $DB_UPL_CONFIG
fi

if [ -f $DB_UPL_STATE ]; then
	echo "$DB_UPL_STATE already existed!!"
	exit 1
else
	echo "Create database ${DB_UPL_STATE}."
	touch $DB_UPL_STATE
fi

tb_sys_config="CREATE TABLE sys_config( \
id			INTEGER not null primary key autoincrement, \
name			TEXT, \
first_use		INTEGER not null default 1, \
network_time		INTEGER not null default 0, \
language		INTEGER not null default 0
);"

echo "Create table 'sys_config'."
echo $tb_sys_config | $SQLITE3_PATH $DB_UPL_CONFIG

sys_config="INSERT INTO sys_config (name, first_use, network_time, language) VALUES ('ugen camera', 1, 0, 0);"

echo "Insert system config to table 'sys_config'."
echo $sys_config | $SQLITE3_PATH $DB_UPL_CONFIG

tb_camera_config="CREATE TABLE camera_config( \
id			INTEGER not null primary key autoincrement, \
cal_times		INTEGER not null default 0, \
cal_interval		INTEGER not null default 0, \
grade			INTEGER not null default 0, \
target_type		INTEGER not null default 1, \
offsetx			REAL not null default 0.0, \
offsety			REAL not null default 0.0, \
thresholdx		REAL not null default 0.0, \
thresholdy		REAL not null default 0.0
);"

echo "Create table 'camera_config'."
echo $tb_camera_config | $SQLITE3_PATH $DB_UPL_CONFIG

camera_config="INSERT INTO camera_config (cal_times, cal_interval, grade, target_type, offsetx, offsety, thresholdx, thresholdy) VALUES (0, 0, 0, 25, 0.0, 0.0, 0.0, 0.0);"

echo "Insert camera config to table 'camera_config'."
echo $camera_config | $SQLITE3_PATH $DB_UPL_CONFIG

tb_template_config="CREATE TABLE template_config( \
id                      INTEGER not null primary key autoincrement, \
name			TEXT, \
coordinate_x		INTEGER not null default 0, \
coordinate_y		INTEGER not null default 0, \
height			INTEGER not null default 100, \
width			INTEGER not null default 100, \
search_range		INTEGER not null default 200, \
mm_per_pix		REAL not null default 0.0, \
sigma			REAL not null default 0.0, \
is_reference		INTEGER not null default 0, \
is_watched		INTEGER not null default 0, \
data			BLOB
);
CREATE UNIQUE INDEX template_idx ON template_config(coordinate_x, coordinate_y);"

echo "Create table 'template_config'."
echo $tb_template_config | $SQLITE3_PATH $DB_UPL_CONFIG

tb_data="CREATE TABLE data( \
id			INTEGER not null primary key autoincrement, \
template_id		INTEGER not null, \
x			REAL not null default 0.0, \
y			REAL not null default 0.0, \
offsetx			REAL not null default 0.0, \
offsety			REAL not null default 0.0, \
timestamp		TEXT
);"

echo "Create table 'data'."
echo $tb_data | $SQLITE3_PATH $DB_UPL_DATA

tb_state="CREATE TABLE state( \
id			INTEGER not null primary key autoincrement, \
saved_state		INTEGER not null default 0, \
init_image		BLOB, \
image_size		INTEGER not null default 0
);"

echo "Create table 'state'."
echo $tb_state | $SQLITE3_PATH $DB_UPL_STATE

init_state="INSERT INTO state(saved_state) VALUES (0);"

echo "Insert init state to table 'state'."
echo $init_state | $SQLITE3_PATH $DB_UPL_STATE

echo "done."
