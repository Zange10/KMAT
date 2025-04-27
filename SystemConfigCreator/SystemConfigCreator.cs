using System;
using UnityEngine;
using System.IO;
using System.Collections.Generic;
using System.Collections;

namespace SystemConfigCreator
{
  [KSPAddon(KSPAddon.Startup.TrackingStation, false)]
	public class SystemConfigCreator : MonoBehaviour
	{
		public void PrintToLog(string msg)
		{ 
			Debug.Log("SystemConfigCreator: " + msg);
		}
		
		public double AngleBetweenVectors(Vector3 v1, Vector3 v2)
		{
			double dot = Vector3.Dot(v1, v2);
			double magProduct = v1.magnitude * v2.magnitude;
			double acosPart = dot / magProduct;

			// Clamp to avoid NaN due to floating-point imprecision
			if (acosPart > 1.0) acosPart = 1.0;
			if (acosPart < -1.0) acosPart = -1.0;

			double angle = Math.Acos(acosPart);
			return Math.Abs(angle);
		}
		
		// only (not actually good) approximation, but very simple model
		public double calcScaleHeight(CelestialBody body)
		{
			float atmoHeight = (float) body.atmosphereDepth;
			float h = atmoHeight / 10f;
			double scaleHeight;
			if (body.atmospherePressureCurveIsNormalized) scaleHeight = - h / Math.Log(body.atmospherePressureCurve.Evaluate(h));
			else scaleHeight = - h / Math.Log(body.atmospherePressureCurve.Evaluate(h) / body.atmospherePressureSeaLevel);
			return scaleHeight;
		}

		public double rad2deg(double rad)
		{
			return rad / Math.PI * 180;
		}
		
		public bool IsRSS()
		{
			return FlightGlobals.Bodies.Exists(b => b.name == "Earth");
		}

		public void store_body_in_file(StreamWriter writer, CelestialBody body)
		{
			writer.WriteLine("[" + body.name + "]");
			
			Color bodyColor = !body.isStar ? body.gameObject.GetComponent<OrbitRendererBase>().nodeColor : new Color(1f, 1f, 0.3f);
			writer.WriteLine("color = [" + bodyColor.r + ", " + bodyColor.g + ", " + bodyColor.b + "]");
			if (body.isHomeWorld) writer.WriteLine("is_homebody = " + body.isHomeWorld);
			writer.WriteLine("gravitational_parameter = " + body.gravParameter);
			writer.WriteLine("radius = " + body.Radius);
			writer.WriteLine("rotational_period = " + body.rotationPeriod);
			if (body.atmosphere)
			{
				writer.WriteLine("sea_level_pressure = " + body.atmospherePressureSeaLevel);
				writer.WriteLine("scale_height = " + calcScaleHeight(body));
				writer.WriteLine("atmosphere_altitude = " + body.atmosphereDepth);
			}

			if (!body.isStar)
			{
				double declination = Math.PI;
				double rightAscension = 0;
				
				Vector3 xVec = Planetarium.right;
				Vector3 yVec = Planetarium.up;
				Vector3 zVec = Planetarium.forward;
				Vector3 northPoleNVector = body.GetWorldSurfacePosition(90, 0, 1e9)-body.GetWorldSurfacePosition(90, 0, 0);
				northPoleNVector.Normalize();
				
				declination = Math.PI / 2 - AngleBetweenVectors(northPoleNVector, yVec);
				Vector3 northPoleNVectorEqProj = new Vector3(northPoleNVector.x, 0, northPoleNVector.z);
				if (northPoleNVectorEqProj.magnitude > 1e-10)
				{
					double anglex = AngleBetweenVectors(northPoleNVectorEqProj, xVec);
					double anglez = AngleBetweenVectors(northPoleNVectorEqProj, zVec);
					rightAscension = anglex;
					if (anglez > Math.PI / 2) rightAscension = 2 * Math.PI - rightAscension;
				}
				Vector3 primeMeridianNVector = body.GetWorldSurfacePosition(0, 0, 1e9)-body.GetWorldSurfacePosition(0, 0, 0);
				primeMeridianNVector.Normalize();
				
				Vector3 bodyXVec = Vector3.Cross(northPoleNVector, zVec);
				Vector3 bodyZVec = Vector3.Cross(bodyXVec, northPoleNVector);
				// Debug.Log("\n" + body.name);
				// Debug.Log("North Vector: " + northPoleNVector*1000);
				// Debug.Log("Prime Meridian Vector: " + primeMeridianNVector*1000);
				
				
				double anglexPm = AngleBetweenVectors(primeMeridianNVector, bodyXVec);
				double anglezPm = AngleBetweenVectors(primeMeridianNVector, bodyZVec);
				double rotation = anglexPm;
				if (anglezPm > Math.PI/2) rotation = 2 * Math.PI - rotation;
				// Using Principia, rotational period seems off. Maybe something to tackle in the future
				double initialRotation = (rotation - (2 * Math.PI) * Planetarium.GetUniversalTime() / body.rotationPeriod) % (2 * Math.PI);
				if (initialRotation < 0) initialRotation += 2 * Math.PI;

				declination = rad2deg(declination);
				rightAscension = rad2deg(rightAscension);
				initialRotation = rad2deg(initialRotation);

				writer.WriteLine("north_pole_declination = " + declination);
				writer.WriteLine("north_pole_right_ascension = " + rightAscension);
				writer.WriteLine("initial_rotation = " + initialRotation);

				Orbit orbit = body.GetOrbit();
				writer.WriteLine("semi_major_axis = " + orbit.semiMajorAxis);
				writer.WriteLine("eccentricity = " + orbit.eccentricity);
				writer.WriteLine("inclination = " + orbit.inclination);
				writer.WriteLine("raan = " + orbit.LAN);
				writer.WriteLine("argument_of_periapsis = " + orbit.argumentOfPeriapsis);
				writer.WriteLine("mean_anomaly_ut0 = " + orbit.getMeanAnomalyAtUT(0));
				writer.WriteLine("parent_body = " + orbit.referenceBody.name);
			}

			writer.WriteLine("");

			foreach (var childBody in body.orbitingBodies)
			{
				store_body_in_file(writer, childBody);
			}
		}
		
		public void store_system_in_file(CelestialBody centralBody, string path, string systemName)
		{
			using (StreamWriter writer = new StreamWriter(path))
			{
				writer.WriteLine("[" + systemName + "]");
				writer.WriteLine("propagation_method = ELEMENTS");
				writer.WriteLine("number_of_bodies = " + (FlightGlobals.Bodies.Count - 1));
				if (IsRSS()) writer.WriteLine("ut0 = 2433647.5000000");
				else writer.WriteLine("ut0 = 0");
				writer.WriteLine("central_body = " + centralBody.name + "\n");

				store_body_in_file(writer, centralBody);
			}
		}
		
		// Coroutine to wait for the GUI and orbits to be ready
		private IEnumerator WaitForTrackingStationGUIAndStoreSystem()
		{
			// PrintToLog("Registered entering tracking station");
			// Wait until the MapView is active (i.e., Tracking Station GUI is visible)
			while (!MapView.MapIsEnabled)
			{
				// PrintToLog("MapView not yet enabled");
				yield return new WaitForSeconds(0.5f);
			}
			// PrintToLog("MapView enabled");
			// The GUI and orbits are ready, you can execute your code now
			yield return new WaitForSeconds(0.5f);
			
			// PrintToLog("Initiating config file creation");
			
			CelestialBody centralBody = FlightGlobals.Bodies[0];
			foreach (var body in FlightGlobals.Bodies)
			{
				if(body.isStar) centralBody = body;
			}
			
			string path = "GameData/KspRoToolsSystemConfigCreator/" + PSystemManager.Instance.systemPrefab.name + ".cfg";
			
			store_system_in_file(centralBody, path, PSystemManager.Instance.systemPrefab.name);
		}
		
		public void Start()
		{
			StartCoroutine(WaitForTrackingStationGUIAndStoreSystem());
		}
	}
}