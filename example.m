function [] = example()
  clean = onCleanup(@() is('close'));

  is('connect', 'amqp://192.168.1.110:30000');
  is('subscribe', 'ptgrey.0.frame');
  is('set_sample_rate', 5.0, {'ptgrey.0', 'ptgrey.1', 'ptgrey.2', 'ptgrey.3'});
  
  for i = 1:100
    img = is('consume_frame', 'ptgrey.0.frame');
  end

end