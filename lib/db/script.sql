CREATE DATABASE IF NOT EXISTS threelight CHARACTER SET utf8;


create table user (
	id BIGINT NOT NULL AUTO_INCREMENT PRIMARY KEY,
	username char(30) NOT NULL UNIQUE,
	name varchar(100),
	surname varchar(100),
	email varchar(100)
);