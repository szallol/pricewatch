-- --------------------------------------------------------
-- Host:                         127.0.0.1
-- Server version:               10.1.8-MariaDB - mariadb.org binary distribution
-- Server OS:                    Win64
-- HeidiSQL Version:             9.1.0.4867
-- --------------------------------------------------------

/*!40101 SET @OLD_CHARACTER_SET_CLIENT=@@CHARACTER_SET_CLIENT */;
/*!40101 SET NAMES utf8mb4 */;
/*!40014 SET @OLD_FOREIGN_KEY_CHECKS=@@FOREIGN_KEY_CHECKS, FOREIGN_KEY_CHECKS=0 */;
/*!40101 SET @OLD_SQL_MODE=@@SQL_MODE, SQL_MODE='NO_AUTO_VALUE_ON_ZERO' */;

-- Dumping database structure for pricewatch
CREATE DATABASE IF NOT EXISTS `pricewatch` /*!40100 DEFAULT CHARACTER SET utf8 COLLATE utf8_unicode_ci */;
USE `pricewatch`;


-- Dumping structure for table pricewatch.categories
CREATE TABLE IF NOT EXISTS `categories` (
  `id` int(10) unsigned NOT NULL AUTO_INCREMENT,
  `shop_id` int(10) unsigned NOT NULL,
  `title` varchar(255) COLLATE utf8_unicode_ci NOT NULL,
  `url` varchar(1024) COLLATE utf8_unicode_ci NOT NULL,
  KEY `if` (`id`),
  KEY `FK__shops` (`shop_id`),
  FULLTEXT KEY `title_url` (`title`,`url`),
  CONSTRAINT `FK__shops` FOREIGN KEY (`shop_id`) REFERENCES `shops` (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8 COLLATE=utf8_unicode_ci;

-- Data exporting was unselected.


-- Dumping structure for table pricewatch.prices
CREATE TABLE IF NOT EXISTS `prices` (
  `id` int(10) unsigned NOT NULL AUTO_INCREMENT,
  `product_id` int(10) unsigned NOT NULL,
  `timestamp_id` int(10) unsigned NOT NULL,
  `price` float DEFAULT '0',
  KEY `id` (`id`),
  KEY `FK__products` (`product_id`),
  KEY `FK__timestamps` (`timestamp_id`),
  CONSTRAINT `FK_prices_products` FOREIGN KEY (`product_id`) REFERENCES `products` (`id`),
  CONSTRAINT `FK_prices_timestamps` FOREIGN KEY (`timestamp_id`) REFERENCES `timestamps` (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8 COLLATE=utf8_unicode_ci;

-- Data exporting was unselected.


-- Dumping structure for table pricewatch.productdetails
CREATE TABLE IF NOT EXISTS `productdetails` (
  `id` int(11) unsigned NOT NULL,
  `identifier` varchar(255) CHARACTER SET utf8 COLLATE utf8_unicode_ci DEFAULT NULL,
  `title` varchar(255) CHARACTER SET utf8 COLLATE utf8_unicode_ci NOT NULL,
  `url` varchar(300) CHARACTER SET utf8 COLLATE utf8_unicode_ci NOT NULL,
  `description` mediumtext CHARACTER SET utf8 COLLATE utf8_unicode_ci,
  `specs` mediumtext CHARACTER SET utf8 COLLATE utf8_unicode_ci,
  `picture` mediumtext CHARACTER SET utf8 COLLATE utf8_unicode_ci,
  UNIQUE KEY `id_unique` (`id`),
  UNIQUE KEY `url` (`url`),
  KEY `id_key` (`id`),
  FULLTEXT KEY `identifier_title_url_description_specs` (`title`,`url`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8;

-- Data exporting was unselected.


-- Dumping structure for table pricewatch.products
CREATE TABLE IF NOT EXISTS `products` (
  `id` int(11) unsigned NOT NULL,
  `shop_id` int(10) unsigned NOT NULL,
  `cat_id` int(10) unsigned NOT NULL,
  `last_update` timestamp NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
  UNIQUE KEY `id_unique` (`id`),
  KEY `FK_products_shops` (`shop_id`),
  KEY `FK_products_categories` (`cat_id`),
  KEY `id_key` (`id`),
  CONSTRAINT `FK_products_categories` FOREIGN KEY (`cat_id`) REFERENCES `categories` (`id`),
  CONSTRAINT `FK_products_shops` FOREIGN KEY (`shop_id`) REFERENCES `shops` (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

-- Data exporting was unselected.


-- Dumping structure for table pricewatch.shops
CREATE TABLE IF NOT EXISTS `shops` (
  `id` int(11) unsigned NOT NULL AUTO_INCREMENT,
  `name` varchar(50) CHARACTER SET utf8 COLLATE utf8_unicode_ci NOT NULL,
  `default_url` varchar(1024) CHARACTER SET utf8 COLLATE utf8_unicode_ci NOT NULL,
  KEY `id` (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

-- Data exporting was unselected.


-- Dumping structure for table pricewatch.timestamps
CREATE TABLE IF NOT EXISTS `timestamps` (
  `id` int(10) unsigned NOT NULL AUTO_INCREMENT,
  `timedate` date NOT NULL,
  `dayofweek` varchar(50) COLLATE utf8_unicode_ci DEFAULT NULL,
  KEY `id` (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8 COLLATE=utf8_unicode_ci;

-- Data exporting was unselected.
/*!40101 SET SQL_MODE=IFNULL(@OLD_SQL_MODE, '') */;
/*!40014 SET FOREIGN_KEY_CHECKS=IF(@OLD_FOREIGN_KEY_CHECKS IS NULL, 1, @OLD_FOREIGN_KEY_CHECKS) */;
/*!40101 SET CHARACTER_SET_CLIENT=@OLD_CHARACTER_SET_CLIENT */;
