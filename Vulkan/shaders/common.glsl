
vec3 ToSRGB(vec3 color){
	return pow(color, vec3(1.0/2.2));
}
