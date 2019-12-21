/*
 *  **********************************************************************
 *  DIEACONU CONFIDENTIAL
 *  _____________________
 *
 *   Copyright 2019 Vlad-Stefan Dieaconu
 *   Zero Rights Reserved.
 *
 *  NOTICE:  All information contained herein is, and remains
 *  the property of Vlad-Stefan Dieaconu. You can use it however you want,
 *  it's OPEN-SOURCE, just don't say it was written by you. Give credits!
 *  Dissemination of this information or reproduction of this material
 *  is strictly approved unless prior written permission is denied by me.
 *  #SharingIsCaring #LongLiveOpenSource #FreeInternet
 *
 *  Original Publisher https://github.com/vladstefandieaconu
 *  Date: November 2019
 *  **********************************************************************
 */
import java.security.MessageDigest;
import java.security.NoSuchAlgorithmException;
import java.util.Set;
import java.nio.charset.StandardCharsets;

/**
 * Class for a space explorer.
 */
public class SpaceExplorer extends Thread {
	// Variables used to make the code look cleaner. As I am not allowed to create new classes for
	// enums and helpers, I am going to put those here
	private final static String EXIT = "EXIT";
	private final static String END = "END";
	private final static boolean INFINITY = true; // used instead of while true

	// Variables recommended by the signature of our constructor method
	private Integer hashCount; // stores the number of times we need to hash the string
	private Set<Integer> discovered; // a data structure needed to not check same solar sys again
	private CommunicationChannel channel; // the communication channel we are using

	/**
	 * Creates a {@code SpaceExplorer} object.
	 *
	 * @param hashCount
	 *            number of times that a space explorer repeats the hash operation
	 *            when decoding
	 * @param discovered
	 *            set containing the IDs of the discovered solar systems
	 * @param channel
	 *            communication channel between the space explorers and the
	 *            headquarters
	 */
	public SpaceExplorer(Integer hashCount, Set<Integer> discovered, CommunicationChannel channel) {
		// Nothing fancy, just a constructor. Access may be private. Won't modify it so I don't ruin
		// the checker, it's sensible as you may know
		this.hashCount = hashCount;
		this.discovered = discovered;
		this.channel = channel;
	}

	@Override
	public void run() {
		// Auxiliary variable to store my message
		Message message;

		// What I do here is to get messages until we receive an END or EXIT
		// while true sounds mainstream so I used INFINITY for a cleaner code
		while (INFINITY) {
			// Be sure to synchronize the channel also when getting messages from it
			synchronized (channel) {
				// Get a new message from the Head Quarter Channel
				message = channel.getMessageHeadQuarterChannel();
			}

			// Check the message. If the message is null or we've already discovered this solar
			// system, we can skip this message and continue with the next one
			if(checkMessage(message)) {
				continue;
			}

			// Here I check if a message containing END or EXIT has finally arrived. If so, we
			// need to interrupt the run function and get the points. The student passed!!
			if(foundExit(message)) {
				return;
			}

			if(message.getData().equals(END)) {
				continue;
			}

			// If message passed all of our verifications, we need to put it on the channel
			// using the hashing function to hash the String value of the message. It could be all
			// done using one one-liner, but it would look kind of hard to understand, so I am using
			// these auxiliary variables
			String valueOfMessage = message.getData();
			
			String hash = encryptMultipleTimes(valueOfMessage, hashCount);
			putMessageOnChannel(message, hash);
		}
	}

	/**
	 * Marks the Solar System as discovered and puts the message on the space explorer channel
	 *
	 * @param message
	 *            the message we want to send, contains the current Solar System infos too
	 *
	 * @param hash
	 *            the encrypted value of the string
	 */
	private void putMessageOnChannel(Message message, String hash) {
		// Our message has a hashed data that needs to be checked before exploring a new solar
		// system. Here we set the value of the hashed data to the message, before it was encrypted
		// multiple times using the encryptMultipleTimes function
		message.setData(hash);

		// Get the current solar system from the message
		Integer currentSolarSystem = message.getCurrentSolarSystem();

		// Mark it as discovered, it's not needed but more efficient to not visit them again
		discovered.add(currentSolarSystem);

		// Put the message on the channel
		channel.putMessageSpaceExplorerChannel(message);
	}

	/**
	 * Helper function, checks if we have a message and if the if was not already discovered
	 *
	 * @param message
	 *            the message we want to check the content of
	 * @return true if it passes our verification
	 */
	private boolean checkMessage(Message message) {
		// If message is null or this Solar System was already discovered, we do not need to
		// discover it again
		if(message == null || discovered.contains(message.getCurrentSolarSystem())) {
			return true;
		}
		// Else continue with this message
		return false;
	}

	/**
	 * Helper function, returns true if we (finally) found the EXIT / END command and we can break
	 * the loop
	 *
	 * @param message
	 *            the message we want to check the content of
	 * @return true if our message's data is equal to EXIT or END, false if not
	 */
	private boolean foundExit(Message message) {
		// In this variable I am going to store the value of the message
		String valueOfMessage = message.getData();

		// If value of message is EXIT or END, we should return true and break the loop
		if(valueOfMessage.equals(EXIT)) {
			return true;
		}
		// Else return false, as we did not found an EXIT command or an END one
		return false;
	}

	/**
	 * Applies a hash function to a string for a given number of times (i.e.,
	 * decodes a frequency).
	 *
	 * @param input
	 *            string to he hashed multiple times
	 * @param count
	 *            number of times that the string is hashed
	 * @return hashed string (i.e., decoded frequency)
	 */
	private String encryptMultipleTimes(String input, Integer count) {
		String hashed = input;
		for (int i = 0; i < count; ++i) {
			hashed = encryptThisString(hashed);
		}

		return hashed;
	}

	/**
	 * Applies a hash function to a string (to be used multiple times when decoding
	 * a frequency).
	 *
	 * @param input
	 *            string to be hashed
	 * @return hashed string
	 */
	private static String encryptThisString(String input) {
		try {
			MessageDigest md = MessageDigest.getInstance("SHA-256");
			byte[] messageDigest = md.digest(input.getBytes(StandardCharsets.UTF_8));

			// convert to string
			StringBuffer hexString = new StringBuffer();
			for (int i = 0; i < messageDigest.length; i++) {
				String hex = Integer.toHexString(0xff & messageDigest[i]);
				if (hex.length() == 1)
					hexString.append('0');
				hexString.append(hex);
			}
			return hexString.toString();

		} catch (NoSuchAlgorithmException e) {
			throw new RuntimeException(e);
		}
	}
}
