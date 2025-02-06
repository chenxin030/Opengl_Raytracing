uniform sampler2D scene;
uniform sampler2D bloomBlur;
uniform float bloomStrength = 0.5;

void main() {
    vec3 sceneColor = texture(scene, TexCoords).rgb;
    vec3 bloomColor = texture(bloomBlur, TexCoords).rgb;
    FragColor = vec4(sceneColor + bloomColor * bloomStrength, 1.0);
}