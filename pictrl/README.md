pictrl is a simple bare-bones web app I wrote in answer to my need for a 
convenient way to perform the handful of basic image management and editing 
operations that I often want to do between taking a digital photo and 
sharing it. 

I've found that I almost always want to do some of the following operations:
* Rotate the image by some multiple of 90 degrees (to correct for 
  portrait/landscape mode)
* Resize the image (because the full high-res version is rarely appropriate, 
  except for printing and archiving)
* Crop the image (even this is rarely essential, but it's a strong
  "good-to-have") 

Whereas, I hardly ever really want to do anything else, meaning every 
image-editing application I've ever seen is extreme overkill for this 
purpose. And the hassle of using them is usually more effort than I care 
to invest, which is why most of my photos end up bit-rotting on my phone 
or camera's memory card. Or at best, copied into an archive folder, never 
to be seen again. 

This version is a sort of "rough draft" -- hacked together as a way of
brainstorming how the app would work. It's written with all logic in 
server-side functional PHP, with static HTML. It can do rotation and 
resizing, as well as transcoding between image formats (but no cropping). 
Even in this rough form, it's already useful for me. But there's 
definitely more I want to do with it. Next steps would be:
* refactor it to be more object-oriented
* separate the UI from the logic, following the MVC pattern
* or, total separation, by building the UI in client-side HTML5/JS, and
  making the server-side PHP component a JSON web-service.
* set up something better than "basic" authentication
* add a "library" view -- ability to specify which images are public, and
  to expose them
* implement cropping
* one more good feature: overlaying text captions

Install Notes:
* it uses the GD image library, so package "php_gd" is required
* code installation:
  * the app's code is self-contained in the pictrl directory, and it stores 
    its data in sub-directories of that. the directory must be owned and
    writable by the webserver user
  * its internal URLs are all relative, so it can be dropped anywhere in the 
    server's docroot. I install it in /var/www/html/pictrl, making the URL
    http://SERVERNAME/pictrl.
* config files:
  * conf.d/z_pictrl.conf goes in Apache config dir (e.g., /etc/httpd/conf.d/)
  * php.d/z_pictrl.ini goes in PHP config dir (e.g., /etc/php.d/)
* authentication:
  * ref. http://httpd.apache.org/docs/2.2/howto/auth.html for setup
  * z_pictrl.conf points to /var/www/passwd/passwords for password file, so
    either create it there or change the config to point to where you want it
    