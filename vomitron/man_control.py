import time
import pygame
from pygame.locals import *
import http.client
import os

class InputEvent:
		def __init__(self, key, down):
				self.key = key
				self.down = down
				self.up = not down

class InputManager:
		def __init__(self):

				self.init_joystick()
				self.buttons = ['up', 'down', 'left', 'right','stop','start','axlock']
								




				self.key_map = {
						K_UP : 'up',
						K_DOWN : 'down',
						K_LEFT : 'left',
						K_RIGHT : 'right',
						K_RETURN : 'stop',
						K_a : 'start',
						K_b : 'axlock'
				}
				


				self.keys_pressed = {}
				for button in self.buttons:
						self.keys_pressed[button] = False


				


				self.joystick_config = {}

				self.joystick_config['up'] = ('is_axis', 1, -1)
				self.joystick_config['down'] = ('is_axis', 1, -1)
				self.joystick_config['left'] = ('is_axis', 0, 1)
				self.joystick_config['right'] = ('is_axis', 0, -1)
				self.joystick_config['stop'] = ('is_button',1)
				self.joystick_config['start'] = ('is_button',0)
				self.joystick_config['axlock'] = ('is_button',3)
				





				self.quit_attempt = False
		

		def is_pressed(self, button):
				return self.keys_pressed[button]
		




		def get_events(self):
				events = []
				for event in pygame.event.get():
						if event.type == QUIT or (event.type == KEYDOWN and event.key == K_ESCAPE):
								self.quit_attempt = True
						
				

				for button in self.buttons:
						

						config = self.joystick_config.get(button)
						if config != None:

								if config[0] == 'is_button':
										pushed = self.joystick.get_button(config[1])
										if pushed != self.keys_pressed[button]:
												events.append(InputEvent(button, pushed))
												self.keys_pressed[button] = pushed
								

								elif config[0] == 'is_axis':
										status = self.joystick.get_axis(config[1])
										
										status = round(status,3)

										pushed = status
										if pushed != self.keys_pressed[button]:
											events.append(InputEvent(button, pushed))
											self.keys_pressed[button] = pushed
								
				return events        
		



		def configure_button(self, button):
				
				js = self.joystick
				


				for axis_index in range(js.get_numaxes()):
						axis_status = js.get_axis(axis_index)
						if axis_status < -.5 and not self.is_axis_used(axis_index, -1):
								self.joystick_config[button] = ('is_axis', 1, -1)
								print(f"LOCK AXIS {axis_index} for {button}")
								return True
						elif axis_status > .5 and not self.is_axis_used(axis_index, 1):
								self.joystick_config[button] = ('is_axis', 0, 1)
								print(f"LOCK AXISGT {axis_index} for {button}")
								return True
								
				return False
		
		
		def is_axis_used(self, axis_index, direction):
				for button in self.buttons:
						config = self.joystick_config.get(button)
						if config != None and config[0] == 'is_axis':
								if config[1] == axis_index and config[2] == direction:
										return True
				return False
		


		def init_joystick(self):
				joystick = pygame.joystick.Joystick(0)
				joystick.init()
				self.joystick = joystick
				self.joystick_name = joystick.get_name()
		
class StickyMcStickerson:
		cooldown = 1
		xmax = 1100
		ymax = 800
		minaxis = 20
		def __init__(self):
				self.x = 320
				self.y = 240
				self.speed = 4
				self.enabled = True
				self.cool = 0
				self.lastx = 0
				self.lasty = 0
				self.xlock = 0
				self.ylock = 0

		def commit(self):
			fs_host = os.environ['FS_HOST']
			fs_port = int(os.environ['FS_PORT'])
			conn = http.client.HTTPConnection(fs_host, fs_host)
			req = ""
			if(self.xlock == 1):
				req = f"/write?axis=roll&r=176&v={self.lastx}"
			elif(self.ylock == 1):
				req = f"/write?axis=pitch&r=176&v={self.lasty}"
			else:
				req = f"/write_dual?rc=176|{self.lastx}&pc=176|{self.lasty}"
			print(req)
			conn.request("GET", req)
			response = conn.getresponse()
			print(response.status)

		
		def move_left(self, speed):
			if(self.enabled and not self.ylock == 1):
				aspeed = int(speed * StickyMcStickerson.xmax)
				if(abs(aspeed) < StickyMcStickerson.minaxis):
					aspeed = 0

				if(aspeed != self.lastx):
					override = (self.lastx != 0 and aspeed == 0)
					self.lastx = aspeed

					if(self.cool <= 0 or override):
						self.commit()
						self.cool = StickyMcStickerson.cooldown
					else:
						self.cool -= 1;

				self.x = 320 + int(speed*150)

		def move_up(self, speed):

			if(self.enabled and not self.xlock == 1):
				aspeed = int(speed*StickyMcStickerson.ymax)
				if(abs(aspeed) < StickyMcStickerson.minaxis):
					aspeed = 0

				if(aspeed != self.lasty):
					override = (self.lasty != 0 and aspeed == 0)
					self.lasty = aspeed
					if(self.cool <= 0 or override):
						self.commit()
						self.cool = StickyMcStickerson.cooldown
					else:
						self.cool -= 1;

				self.y = 240 + int(speed*150)

def main():
		
		fps = 30

		pygame.init()
		
		num_joysticks = pygame.joystick.get_count()
		if num_joysticks < 1:
				print("You didn't plug in a joystick. FORSHAME!")
				return
		
		input_manager = InputManager()
		
		screen = pygame.display.set_mode((640, 480))
		
		button_index = 0
		
		myjoystick = StickyMcStickerson()
		
		

		while not input_manager.quit_attempt:
				start = time.time()
				
				screen.fill((0,0,0))
				

				interaction_phase(screen, myjoystick, input_manager)
				
				pygame.display.flip()
				

				difference = start - time.time()
				delay = 1.0 / fps - difference
				if delay > 0:
						time.sleep(delay)

def configure_phase(screen, button, input_manager):
		

		input_manager.get_events() 
		



		success = input_manager.configure_button(button)
		

		write_text(screen, "Press the " + button + " button", 100, 100)
		

		return success

def interaction_phase(screen, myjoystick, input_manager):




		for event in input_manager.get_events():
				if event.key == 'A' and event.down:

				if event.key == 'X' and event.up:
						input_manager.quit_attempted = True
				if event.key == 'axlock' and event.up:

					if(myjoystick.ylock == 0 and myjoystick.xlock == 0):
						myjoystick.xlock = 1
					elif(myjoystick.xlock == 1):
						myjoystick.ylock = 1
						myjoystick.xlock = 0
					else:
						myjoystick.xlock = 0
						myjoystick.ylock = 0



		
		if input_manager.is_pressed('left'):
				myjoystick.move_left(input_manager.is_pressed('left'))
		elif input_manager.is_pressed('right'):
				myjoystick.move_right(input_manager.is_pressed('right'))
		if input_manager.is_pressed('up'):
				myjoystick.move_up(input_manager.is_pressed('up'))
		elif input_manager.is_pressed('down'):
				myjoystick.move_down(input_manager.is_pressed('down'))
		if input_manager.is_pressed('stop'):	
				if(myjoystick.lastx != 0 or myjoystick.lasty != 0):
					myjoystick.lastx = myjoystick.lasty = 0
					myjoystick.commit()	
				myjoystick.enabled = False
		if input_manager.is_pressed('start'):		
				myjoystick.enabled = True
		

		rval = 0
		bval = 0
		if(myjoystick.enabled):
			rval = 0 
			bval = 255
		else:
			rval = 255
			bval = 0

		lock_label = "Free Axis"
		if(myjoystick.xlock == 1):
			lock_label = "Locked to Roll only"
		elif(myjoystick.ylock == 1):
			lock_label = "Locked to Pitch only"

		en_txt = "ENABLED" if myjoystick.enabled else "DISABLED"
		write_text(screen, lock_label, 30, 30)
		write_text(screen, en_txt, 500, 30)

		pygame.draw.circle(screen, (rval, 0, bval), (myjoystick.x, myjoystick.y), 20)

def wait_for_enter():
		try: input()
		except: pass

cached_text = {}
cached_font = None
def write_text(screen, text, x, y):
		global cached_text, cached_font
		image = cached_text.get(text)
		if image == None:
				if cached_font == None:
						cached_font = pygame.font.Font(pygame.font.get_default_font(), 17)
				image = cached_font.render(text, True, (255, 255, 255))
				cached_text[text] = image
		screen.blit(image, (x, y - image.get_height()))

main()