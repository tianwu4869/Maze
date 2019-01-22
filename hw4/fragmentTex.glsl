#version 150 core

in vec3 Color;
in vec3 normal;
in vec3 pos;
in vec3 lightDir;
in vec2 texcoord;

out vec4 outColor;

uniform sampler2D tex0;
uniform sampler2D tex1;

uniform int texID;
uniform int getKey;
uniform vec3 cam_position;

const float ambient = .3;
void main() {
  vec3 color;
  vec3 whiteLight = vec3(1.0, 1.0, 1.0);
  if (texID == 0)
    color = texture(tex0, texcoord).rgb;
  else if (texID == 1)
    color = texture(tex1, texcoord).rgb;  
  else if (texID == -1){
   	color = vec3(0.5,0.5,0.3);
     if(getKey == 1){
       outColor = vec4(color, 1);
       return;
     }
  }
  else if (texID == -2){
   	color = vec3(0.7,0.1,0.3);
     if(getKey == 1){
       outColor = vec4(color, 1);
       return;
     }
  }
  else if (texID == -3){
   	color = vec3(0.7,0.5,0.1);
     if(getKey == 1){
       outColor = vec4(color, 1);
       return;
     }
  }
  else if (texID == -4){
   	color = vec3(0.7,0.5,0.6);
     if(getKey == 1){
       outColor = vec4(color, 1);
       return;
     }
  }
  else if (texID == -5){
   	color = vec3(0.2,0.5,0.3);
     if(getKey == 1){
       outColor = vec4(color, 1);
       return;
     }
  }
   float distance = sqrt(pow(cam_position.x - pos.x, 2) + pow(cam_position.y - pos.y, 2) + pow(cam_position.z - pos.z, 2));
   vec3 diffuseC = color*max(dot(-lightDir,normal),0.0);
   vec3 ambC = color*ambient;
   vec3 viewDir = normalize(cam_position-pos); //We know the eye is at (0,0)!
   vec3 reflectDir = reflect(viewDir,normal);
   float spec = max(dot(reflectDir,lightDir),0.0);
   float spec2 = max(dot(reflectDir,cam_position-pos),0.0);
   if (dot(-lightDir,normal) <= 0.0)spec = 0;
    if (dot(cam_position-pos,normal) <= 0.0)spec2 = 0;
   vec3 specC = .8*vec3(1.0,1.0,1.0)*pow(spec,4);
   vec3 oColor = ambC+diffuseC+specC;
   if(oColor.x >= 1.0){
     oColor.x == 1.0;
   }
   if(oColor.y >= 1.0){
     oColor.y == 1.0;
   }
   if(oColor.z >= 1.0){
     oColor.z == 1.0;
   }
	   outColor = vec4(oColor ,1);
}