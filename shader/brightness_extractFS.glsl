uniform sampler2D hdrTexture;
uniform float threshold = 1.0;

layout(location = 0) out vec4 brightColor;

void main() {
    vec3 hdrColor = texture(hdrTexture, TexCoords).rgb;
    float brightness = dot(hdrColor, vec3(0.2126, 0.7152, 0.0722));
    if(brightness > threshold) {
        brightColor = vec4(hdrColor, 1.0);
    } else {
        brightColor = vec4(0.0, 0.0, 0.0, 1.0);
    }
}