is('close');
is('connect', 'amqp://192.168.1.110:30000');
is('subscribe', 'ptgrey.0.frame');

img = is('consume_frame', 'ptgrey.0.frame');
imshow(img)