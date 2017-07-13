function [] = example()
  clean = onCleanup(@() is('close'));

  is('connect', 'amqp://192.168.1.110:30000');
  bbs = [200.0 300.0 50.0 200.0 10.5 2;
				 300.0 300.0 80.0 100.0 0.125 1.0;
				 400.0 200.0 60.0 150.0 825.36 3;
				 600.0 400.0 75.0 300.0 25.36 4]; % x, y, w, h, score, camera_id
  for i=1:1000
		tic;
	  is('publish_bbs', 'ptgrey.0.bbs', bbs);
	  is('publish_bbs', 'ptgrey.1.bbs', bbs);
	  is('publish_bbs', 'ptgrey.2.bbs', bbs);
	  is('publish_bbs', 'ptgrey.3.bbs', bbs);
		pause(0.2 - toc); % period = 200ms
  end