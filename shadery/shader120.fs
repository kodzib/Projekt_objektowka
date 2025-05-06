#version 120

// Input vertex attributes (from vertex shader)
varying vec3 fragPosition;
varying vec2 fragTexCoord;
varying vec3 fragNormal;
varying vec4 fragPosLightSpace; // passed from vertex shader

// Uniforms
uniform sampler2D texture0;
uniform vec4 colDiffuse;

uniform vec3 lightDir;
uniform vec4 lightColor;
uniform vec4 ambient;
uniform vec3 viewPos;

uniform sampler2D shadowMap;
uniform int shadowMapResolution;

void main()
{
    vec4 texelColor = texture2D(texture0, fragTexCoord);
    vec3 normal = normalize(fragNormal);
    vec3 viewD = normalize(viewPos - fragPosition);
    vec3 lightVec = -lightDir;

    // Lighting
    float NdotL = max(dot(normal, lightVec), 0.0);
    vec3 diffuse = lightColor.rgb * NdotL;
    float specCo = (NdotL > 0.0) ? pow(max(dot(viewD, reflect(-lightVec, normal)), 0.0), 16.0) : 0.0;
    vec3 specular = vec3(specCo);

    vec4 finalColor = texelColor * ((colDiffuse + vec4(specular, 1.0)) * vec4(diffuse, 1.0));

    // Light-space projection from vertex shader
    vec4 projCoords = fragPosLightSpace;
    projCoords.xyz /= projCoords.w;
    projCoords.xyz = projCoords.xyz * 0.5 + 0.5;

    vec2 sampleCoords = projCoords.xy;
    float currentDepth = projCoords.z;

    // Bias to reduce shadow acne
    float bias = max(0.0008 * (1.0 - dot(normal, lightVec)), 0.00008);

    // PCF (5x5)
    const int kernelSize = 2;
    const int numSamples = (2 * kernelSize + 1) * (2 * kernelSize + 1);
    vec2 texelSize = vec2(1.0 / float(shadowMapResolution));

    int shadowCount = 0;
    for (int x = -kernelSize; x <= kernelSize; x++)
    {
        for (int y = -kernelSize; y <= kernelSize; y++)
        {
            vec2 offset = texelSize * vec2(float(x), float(y));
            vec2 clampedCoords = clamp(sampleCoords + offset, 0.0, 1.0);
            float sampleDepth = texture2D(shadowMap, clampedCoords).r;
            if (currentDepth - bias > sampleDepth)
            {
                shadowCount++;
            }
        }
    }

    float shadowFactor = float(shadowCount) / float(numSamples);
    shadowFactor = smoothstep(0.0, 1.0, shadowFactor);

    // Brightness-based shadow blending
    float brightness = mix(1.0, 0.3, shadowFactor);
    finalColor.rgb *= brightness;

    // Ambient contribution
    vec3 ambientLight = texelColor.rgb * (ambient.rgb * colDiffuse.rgb);
    finalColor.rgb = mix(finalColor.rgb, ambientLight, shadowFactor * 0.5);

    // Gamma correction
    finalColor = pow(finalColor, vec4(1.0 / 2.2));
    gl_FragColor = finalColor;
}
