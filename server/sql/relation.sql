/* This file was generated by ODB, object-relational mapping (ORM)
 * compiler for C++.
 */
CREATE DATABASE IF NOT EXISTS `crin_lc`;
USE `crin_lc`;

DROP TABLE IF EXISTS `relation`;

CREATE TABLE `relation` (
  `id` BIGINT UNSIGNED NOT NULL PRIMARY KEY AUTO_INCREMENT,
  `user_id` varchar(64) NOT NULL,
  `peer_id` varchar(64) NOT NULL)
 ENGINE=InnoDB;

CREATE INDEX `user_id_i`
  ON `relation` (`user_id`);

