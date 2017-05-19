function [] = example()
  clean = onCleanup(@() is('close'));

  is('connect', 'amqp://192.168.1.110:30000');
  is('subscribe', 'ptgrey.0.frame');

  for i = 1:1000
    img = is('consume_frame', 'ptgrey.0.frame');
  end

end