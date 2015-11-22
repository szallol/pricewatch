select * from (select * from productdetails where MATCH(title, url) AGAINST ('xerox') ) as selectedproducts
	 left join products on  selectedproducts.id = products.id 
	 left join categories on products.cat_id = categories.id
	 
select * from (select * from products where cat_id=152) as selectedproducts
	 left join productdetpricewatchails on  selectedproducts.id = productdetails.id 
	 left join categories on selectedproducts.cat_id = categories.id
select *, count(*) as c from productdetails group by url HAVING c>1 order by c desc

SELECT productdetails.id, products.cat_id, title, url, shop_id, last_update  from products 
		LEFT JOIN productdetails ON products.id=productdetails.id WHERE last_update IS NOT NULL # select updated products 

SELECT id, title, url,  COUNT(*) c FROM productdetails GROUP BY url HAVING c > 1 ORDER BY c DESC # find duplicates

SELECT id, title, url FROM productdetails WHERE url ="http://www.emag.ro/televizor-smart-led-lg-109-cm-4k-ultra-hd-43uf6407/pd/DLRKVMBBM/"
