// def pitch_test_1
// 	(1..89).to_a + (1..88).to_a.reverse + (-89..-1).to_a.reverse + (-88 .. -1).to_a + (1..40).to_a
// end

// def pitch_expected_1
// 	(1..359).to_a
// end

// def roll_test_1
// 	(1..179).to_a + (-179 .. -1).to_a
// end

// def roll_expected_1
// 	(1..359).to_a
// end

// def pitch_rand

// end

// def the_test
// 	pitch_test_1.each do |n|
// 	  rpm, cpp = process_pitch_coord(n)
// 	  puts "RPM #{rpm}, cpp #{cpp} for input #{n} lRPM:#{$last_pitch_rpm}"
// 	  sleep(0.01)
// 	end
// end

#include <sys/time.h>

static int RPM_direction_LOCK = 4;
static int TRANSITION_ZONE = 10;
static int TDIFF_DISREGARD = 5000;

int last_pitch_fscoord = 0;
int last_pitch_position = 0;
int last_pitch_rpm = 0;
int last_pitch_time = 0;
int last_pitch_region = 1;

int last_roll_fscoord = 0;
int last_roll_position = 0;
int last_roll_time = 0;
int last_roll_rpm = 0;
int last_roll_region = 0;

int convert_fs_pitch_to_360(int fs_pitch){
	int region = 1;

	if(last_pitch_region == 1){
		if(fs_pitch < 0){
			region = 4;
		}else if(fs_pitch < 90 && fs_pitch < last_pitch_fscoord && fs_pitch > 90 - TRANSITION_ZONE && last_pitch_rpm > RPM_direction_LOCK){
			region = 2;
		}else if(fs_pitch < 90){
			region = 1;
		}
	}
	else if(last_pitch_region == 2){
		if(fs_pitch < 0){
			region = 3;
		}else if(fs_pitch < 90 && fs_pitch < last_pitch_fscoord && fs_pitch > 90 - TRANSITION_ZONE && last_pitch_rpm < -RPM_direction_LOCK){
			region = 1;
		}else if(fs_pitch < 90){
			region = 2;
		}
	}else if(last_pitch_region == 3){
		if(fs_pitch > 0){
			region = 2;
		}else if(fs_pitch > -90 && fs_pitch > last_pitch_fscoord && fs_pitch < -90 + TRANSITION_ZONE && last_pitch_rpm > RPM_direction_LOCK){
			region = 4;
		}else if(fs_pitch < 90){
			region = 3;
		}
	}else if(last_pitch_region == 4){
		if(fs_pitch > 0){
			region = 1;
		}else if(fs_pitch > -90 && fs_pitch > last_pitch_fscoord && fs_pitch < -90 + TRANSITION_ZONE && last_pitch_rpm < -RPM_direction_LOCK){
			region = 3;
		}else if(fs_pitch < 0){
			region = 4;
		}
	}


  last_pitch_fscoord = fs_pitch;
  last_pitch_region = region;

  switch(region){
  	case 1:
  	  return fs_pitch;
  	case 2:
  	  return 90 + (90 - fs_pitch);
  	case 3:
  	  return 180 - fs_pitch;
  	case 4:
  	  return 360 + fs_pitch;
  }
}

int current_unix_timestamp(){
	struct timeval current_time;
    gettimeofday(&current_time, NULL);
    return (current_time.tv_sec * 1000) + (current_time.tv_usec/1000);
}


void process_pitch_coord(int *rpm, int *current_pitch_position){

	int current_pitch_position_tmp = convert_fs_pitch_to_360(pc);

	uint8_t direction = 1;
	int position_change = 0;

	if(current_pitch_position_tmp < 50 && last_pitch_position > 300){
		position_change = (360 - last_pitch_position) + current_pitch_position_tmp;
	}else if(current_pitch_position_tmp > 300 && last_pitch_position < 50){
		position_change = (360 - current_pitch_position_tmp) + last_pitch_position;
		direction = -1;
	}else{
		direction = ((last_pitch_position - current_pitch_position_tmp) > 0) ? -1 : 1;
		position_change = abs(last_pitch_position - current_pitch_position_tmp);
	}

	int time_difference = 0;

	int current_unix_time = current_unix_timestamp();

	if(last_pitch_time == 0){
		last_pitch_time = current_unix_time - 1000; //Need something... why not a second ago
	}

	int time_difference = current_unix_time - last_pitch_time;
	last_pitch_time = current_unix_time;

	int current_rpm;
	current_rpm = ((position_change / 360.0) * (60000/time_difference)) * direction;

	last_pitch_rpm = current_rpm;
	last_pitch_position = current_pitch_position_tmp;

	*rpm = current_rpm;
	*current_pitch_position = current_pitch_position_tmp;
}

int convert_fs_roll_to_360(int fs_roll){
	int region = 1;

	if(last_roll_region == 1){
		if(fs_roll < 0){
			region = 4;
		}else if(fs_roll < 90 && fs_roll < last_roll_fscoord && fs_roll > 90 - TRANSITION_ZONE && last_roll_rpm > RPM_direction_LOCK){
			region = 2;
		}else if(fs_roll < 90){
			region = 1;
		}
	}
	else if(last_roll_region == 2){
		if(fs_roll < 0){
			region = 3;
		}else if(fs_roll < 90 && fs_roll < last_roll_fscoord && fs_roll > 90 - TRANSITION_ZONE && last_roll_rpm < -RPM_direction_LOCK){
			region = 1;
		}else if(fs_roll < 90){
			region = 2;
		}
	}


  last_roll_fscoord = fs_roll;
  last_roll_region = region;

  switch(region){
  	case 1:
  	  return fs_roll;
  	case 2:
  	  return 90 + (90 - fs_roll);
  	case 3:
  	  return 180 - fs_roll;
  	case 4:
  	  return 360 + fs_roll;
  }
}

void process_roll_coord(int *rpm, int *current_roll_position){

	int current_roll_position_tmp = convert_fs_roll_to_360(pc);

	uint8_t direction = 1;
	int position_change = 0;

	if(current_roll_position_tmp < 50 && last_roll_position > 300){
		position_change = (360 - last_roll_position) + current_roll_position_tmp;
	}else if(current_roll_position_tmp > 300 && last_roll_position < 50){
		position_change = (360 - current_roll_position_tmp) + last_roll_position;
		direction = -1;
	}else{
		direction = ((last_roll_position - current_roll_position_tmp) > 0) ? -1 : 1;
		position_change = abs(last_roll_position - current_roll_position_tmp);
	}

	int time_difference = 0;

	int current_unix_time = current_unix_timestamp();

	if(last_roll_time == 0){
		last_roll_time = current_unix_time - 1000; //Need something... why not a second ago
	}

	int time_difference = current_unix_time - last_roll_time;
	last_roll_time = current_unix_time;

	int current_rpm;
	current_rpm = ((position_change / 360.0) * (60000/time_difference)) * direction;

	last_roll_rpm = current_rpm;
	last_roll_position = current_roll_position_tmp;

	*rpm = current_rpm;
	*current_roll_position = current_roll_position_tmp;
}