function [] = example()
  clean = onCleanup(@() is('close'));

  is('connect', 'amqp://192.168.1.110:30000');
  is('subscribe', {'ptgrey.0.frame', 'ptgrey.1.frame', 'ptgrey.2.frame', 'ptgrey.3.frame'});
  is('set_sample_rate', {'ptgrey.0', 'ptgrey.1', 'ptgrey.2', 'ptgrey.3'}, 5.0);

  if (is('sync_request', {'ptgrey.0', 'ptgrey.1', 'ptgrey.2', 'ptgrey.3'}, 5.0))
    disp('Sync done!')
  else
    disp('Sync failed!')
  end
  
  for i = 1:10
    imgs = is('consume_frames_sync', {'ptgrey.0.frame', 'ptgrey.1.frame', 'ptgrey.2.frame', 'ptgrey.3.frame'}, 5.0);
  end

end