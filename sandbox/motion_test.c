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

// RPM_DIR_LOCK = 1 #If we're over this RPM, do not allow reversal of direction. Higher can allow more violent transitions
// TRANSITION_ZONE = 10 #The area before the switch threshold that we allow switching regions
// TDIFF_DISREGARD = 50000 #ms to use as a cutoff for commands. If we've waited this long, it's possibly dangerous to jump.

static int RPM_DIR_LOCK = 4;
static int TRANSITION_ZONE = 10;
static int TDIFF_DISREGARD = 5000;
// $last_pitch_fscoord = 0
// $last_pitch_position = 0
// $last_pitch_rpm = 0
// $last_pitch_time = 0
// $last_pitch_region = 1
int last_pitch_fscoord = 0;
int last_pitch_position = 0;
int last_pitch_rpm = 0;
int last_pitch_time = 0;
int last_pitch_region = 1;

// $last_roll_fscoord = 0
// $last_roll_position = 0
// $last_roll_rpm = 0
// $last_roll_time = 0
// $last_roll_region = 1
int last_roll_fscoord = 0;
int last_roll_position = 0;
int last_roll_time = 0;
int last_roll_rpm = 0;
int last_roll_region = 0;

int convert_fs_pitch_to_360(int fs_pitch){
	int region = 1;

	if(last_pitch_region == 1){

	}
	else if(last_pitch_region == 2){

	}else if(last_pitch_region == 3){

	}else if(last_pitch_region == 4){

	}

    
// 	#Region 1
// 	if($last_pitch_region == 1)
// 	 #If fs_pitch is  < 0 we've transitioned to R4
// 	 if fs_pitch < 0
// 	 	 puts "REGION TRANSFER 1 to 4"
// 	 	 region = 4
// 	 elsif fs_pitch < 90 && fs_pitch < $last_pitch_fscoord && fs_pitch > 90 - TRANSITION_ZONE && $last_pitch_rpm > RPM_DIR_LOCK
// 	 	puts "REGION TRANSFER 1 to 2"
// 	   region = 2
// 	 elsif fs_pitch < 90
// 	   region = 1
// 	 end

// 	#Region 2
// 	elsif($last_pitch_region == 2)
// 		#If RPM is greater than jump threshold and we're descending, transition to R1
// 		#If fs_pitch is < 0 we've transitioned to R3
// 		if fs_pitch < 0
// 		  region = 3
// 		  puts "REGION TRANSFER 2 to 3"
// 		elsif fs_pitch < 90 && fs_pitch < $last_pitch_fscoord && fs_pitch > 90 - TRANSITION_ZONE && $last_pitch_rpm  < -RPM_DIR_LOCK
// 			puts "REGION TRANSFER 2 to 1"
// 		  region = 1
// 		elsif fs_pitch < 90
// 		  region = 2
// 		end

// 	#Region 3
// 	elsif($last_pitch_region == 3)
// 		#If fs_pitch is > 0 we've transitioned to R2
// 		#If RPM is greater than jump threshold and we're ascending, transition to R4
// 		if fs_pitch > 0
// 		  region = 2
// 		elsif fs_pitch > -90 && fs_pitch > $last_pitch_fscoord && fs_pitch > -90 + TRANSITION_ZONE && $last_pitch_rpm  < RPM_DIR_LOCK
// 		  region = 4
// 		elsif fs_pitch < 0
// 		  region = 3
// 		end

// 	#Region 4
// 	elsif($last_pitch_region == 4)
// 		#If fs_pitch is >0 we've transitioned to R1
// 		#If #If RPM is greater than jump threshold and we're descending, transition to R3

// 		if fs_pitch > 0
// 		  region = 1
// 		elsif fs_pitch > -90 && fs_pitch > $last_pitch_fscoord && fs_pitch > -90 + TRANSITION_ZONE && $last_pitch_rpm  < RPM_DIR_LOCK
// 		  region = 3
// 		elsif fs_pitch < 0
// 		  region = 4
// 		end
// 	end

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


// def process_pitch_coord(pc)
 
//   curr_pitch_position = convert_fs_pitch_to_360(pc)

//    ##JUST RPM STUFF THAT WILL BE DEPRECATED ONCE PC IS FIGURED OUT
//    dir = 1
//    pos_delta = if(curr_pitch_position < 50 && $last_pitch_position > 300)
//    	(360 - $last_pitch_position) + curr_pitch_position
//    elsif(curr_pitch_position > 300 && $last_pitch_position < 50)
//    	dir = -1
//    	(360 - curr_pitch_position) + $last_pitch_position
//    else
//    	dir = ($last_pitch_position - curr_pitch_position) > 0 ? -1 : 1
//    	($last_pitch_position - curr_pitch_position).abs
//    end

//    timediff = 0 

//    ctime = (Time.now.to_f * 1000).to_i
//    timediff = ctime - $last_pitch_time
//    $last_pitch_time = ctime
   
//    # not the first we've heard
//    rpm = if timediff != 0 && timediff < TDIFF_DISREGARD
//    	((pos_delta / 360.0) * (60000/timediff)) * dir
//    end


//    $last_pitch_rpm = if rpm
//    	rpm = rpm.round(1)
//    	rpm
//    else
//    	0
//    end
//    $last_pitch_position = curr_pitch_position

//    return rpm, curr_pitch_position

// end

// def convert_fs_roll_to_360(fs_roll)
// 	region = 1

// 	#Region 1
// 	if($last_roll_region == 1)
// 	 #If fs_roll is  < 0 we've transitioned to R4
// 	 if fs_roll < 0
// 	 	 puts "REGION TRANSFER 1 to 4"
// 	 	 region = 4
// 	 elsif fs_roll < 90 && fs_roll < $last_roll_fscoord && fs_roll > 90 - TRANSITION_ZONE && $last_roll_rpm > RPM_DIR_LOCK
// 	 	puts "REGION TRANSFER 1 to 2"
// 	   region = 2
// 	 elsif fs_roll < 90
// 	   region = 1
// 	 end

// 	#Region 2
// 	elsif($last_roll_region == 2)
// 		#If RPM is greater than jump threshold and we're descending, transition to R1
// 		#If fs_roll is < 0 we've transitioned to R3
// 		if fs_roll < 0
// 		  region = 3
// 		  puts "REGION TRANSFER 2 to 3"
// 		elsif fs_roll < 90 && fs_roll < $last_roll_fscoord && fs_roll > 90 - TRANSITION_ZONE && $last_roll_rpm  < -RPM_DIR_LOCK
// 			puts "REGION TRANSFER 2 to 1"
// 		  region = 1
// 		elsif fs_roll < 90
// 		  region = 2
// 		end

//     end

// 	$last_roll_fscoord = fs_roll
// 	$last_roll_region = region

// 	case region
// 	  when 1
// 	    fs_roll
// 	  when 2
// 	    90 + (90 - fs_roll)
// 	 end
// end


// def process_roll_coord(pc)
 
//   curr_roll_position = convert_fs_roll_to_360(pc)

//    ##JUST RPM STUFF THAT WILL BE DEPRECATED ONCE PC IS FIGURED OUT
//    dir = 1
//    pos_delta = if(curr_roll_position < 50 && $last_roll_position > 300)
//    	(360 - $last_roll_position) + curr_roll_position
//    elsif(curr_roll_position > 300 && $last_roll_position < 50)
//    	dir = -1
//    	(360 - curr_roll_position) + $last_roll_position
//    else
//    	dir = ($last_roll_position - curr_roll_position) > 0 ? -1 : 1
//    	($last_roll_position - curr_roll_position).abs
//    end

//    timediff = 0 

//    ctime = (Time.now.to_f * 1000).to_i
//    timediff = ctime - $last_roll_time
//    $last_roll_time = ctime
   
//    # not the first we've heard
//    rpm = if timediff != 0 && timediff < TDIFF_DISREGARD
//    	((pos_delta / 360.0) * (60000/timediff)) * dir
//    end


//    $last_roll_rpm = if rpm
//    	rpm = rpm.round(1)
//    	rpm
//    else
//    	0
//    end
//    $last_roll_position = curr_roll_position

//    return rpm, curr_roll_position

// end

// puts "STARTING"
// the_test