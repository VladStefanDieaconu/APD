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

import java.util.LinkedList;
import java.util.Map;
import java.util.concurrent.*;

/**
 * Class that implements the channel used by headquarters and space explorers to communicate.
 */
public class CommunicationChannel {
	// Locks used for synchronization
	private final Object lock1; // used inside putMessageHeadQuarterChannel()
	private final Object lock2; // used inside getMessageHeadQuarterChannel()
	private final Object lock3; // used inside getMessageHeadQuarterChannel()

	// Variables used to make the code look cleaner. As I am not allowed to create new classes for
	// enums and helpers, I am going to put those here
	private final static String EXIT = "EXIT";
	private final static String END = "END";

	// Here I am going to store the current thread ID
	private long threadID;

	// Where Space Explorers put messages and Head Quarters read from
	private static SynchronousQueue<Message> messagesSpaceExplorers;

	// Where Head Quarters put messages and Space Explorers read from
	private ConcurrentHashMap<Long, LinkedList<Message>> messagesHeadQuarters;

	// Some Object Oriented Programming, not needed but I think it looks better using
	// those getters and setters
	public long getThreadID() {
		return threadID;
	}

	public void setThreadID(long threadID) {
		this.threadID = threadID;
	}

	/**
	 * Creates a {@code CommunicationChannel} object.
	 */
	public CommunicationChannel() {
		// Initialization of the objects I am going to use to synchronize my methods
		lock1 = new Object();
		lock2 = new Object();
		lock3 = new Object();

		// Basic initializations, we need to use new before adding anything to those data structures
		// Could have been done at declaration, but looks better here. Explicit type argument is not
		// needed, I am doing it as a good behaviour
		messagesSpaceExplorers = new SynchronousQueue<Message>();
		messagesHeadQuarters = new ConcurrentHashMap<Long, LinkedList<Message>>();
	}

	/**
	 * Puts a message on the space explorer channel (i.e., where space explorers write to and
	 * headquarters read from).
	 *
	 * @param message
	 *            message to be put on the channel
	 */
	public void putMessageSpaceExplorerChannel(Message message) {
		// Before putting a message, try to see if the message is null, maybe something bad
		// happend. Also this could be synchronized as the other methods, but it would make no
		// no difference, as I am using a SynchronousQueue, that already has built-in
		// synchronization
		if (message == null) {
			return; // something is wrong!!!
		} else {
			// By using a Blocking Queue we do not have to synchronize it ourselves
			// as it's already synchronized using locks
			try {
				messagesSpaceExplorers.put(message);
			} catch (InterruptedException e) {
				// Something had just happend, found an InterruptedException
			}
		}
	}

	/**
	 * Gets a message from the space explorer channel (i.e., where space explorers write to and
	 * headquarters read from).
	 *
	 * @return message from the space explorer channel
	 */
	public Message getMessageSpaceExplorerChannel() {
		// Initialization of the message with null (because at first we do not have a message)
		// We only have the message if the get succeeded
		Message message = null;

		// By using a Blocking Queue we do not have to synchronize it, as it's already synchronized
		// Synchronizing this using the same mechanism used for the putMessageHeadQuarterChannel and
		// getMessageHeadQuarterChannel would result in a deadlock
		try {
			message = messagesSpaceExplorers.take();
		} catch (InterruptedException e) {
			// Found an InterruptedException
		}
		// If try block did not succeed value will be null
		return message;
	}

	/**
	 * Puts a message on the headquarters channel (i.e., where headquarters write to and
	 * space explorers read from).
	 *
	 * @param message
	 *            message to be put on the channel
	 */
	void putMessageHeadQuarterChannel(Message message) {
		// Synchronize it using locks on objects
		synchronized (lock1) {
			// Get the current thread iD of the Head Quarter
			setThreadID(Thread.currentThread().getId());

			if(messagesHeadQuarters.containsKey(getThreadID())) {
				// But before adding the message, we need to check if the message has redundant
				// information.
				if (!foundEnd(message)) {
					messagesHeadQuarters.get(getThreadID()).add(message);
				}

				if (foundExit(message)) {
					messagesHeadQuarters.get(getThreadID()).add(message);
				}
			} else {
				// Create a new entry and add it to the map
				LinkedList<Message> newMessage = new LinkedList<Message>();
				newMessage.add(message);

				// Finally, add the message to the Head Quarters buffer
				messagesHeadQuarters.put(getThreadID(), newMessage);
			}
		}
	}

	/**
	 * Gets a message from the headquarters channel (i.e., where headquarters write to and
	 * space explorer read from).
	 *
	 * @return message from the header quarter channel
	 */
	public Message getMessageHeadQuarterChannel() {
		// Set the parent to -inf, so in case anything goes bad we can verify against this corner
		// case and return an error or throw something
		int parentSolarSystem = Integer.MIN_VALUE;

		// The value of the message stays null until the get succeeded
		Message nextMessage = null;

		// Synchronize method using locks on objects
		synchronized (lock2) {
			// Here we apply the same algorithm used for our put method. Only difference is that
			// we want to get a message, so we need to do one synchronization in addition
			for (Map.Entry<Long, LinkedList<Message>> entry : messagesHeadQuarters.entrySet()) {
				// To be sure that we didn't added another element to our list. This is done
				// because we are interested just in those that have 2 or more elements, meaning
				// they contain at least one parent and one child
				synchronized (lock3) {
					LinkedList<Message> list = entry.getValue();

					// If the condition was not satisfied and we do not have a parent and a child,
					// continue with the next list
					if(list.size() < 2) {
						continue;
					}

					// Get the value of the parent
					parentSolarSystem =  list.removeFirst().getCurrentSolarSystem();

					// Get the next element (meaning the child)
					nextMessage = list.removeFirst();

					// Here I verify the corner case I was talking at the beginning of this method's
					// implementation
					if(parentSolarSystem != Integer.MIN_VALUE) {
						// Set the parent of this child
						nextMessage.setParentSolarSystem(parentSolarSystem);
					}
					// We found what we were looking for, just break as we do not need to iterate
					// through what remains
					break;
				}
			}
		}
		// Return the next message. If anything bad happens and the get method does not succeed,
		// the value of next is going to be null
		return nextMessage;
	}

	/**
	 * Helper function, returns true if we found EXIT
	 *
	 * @param message
	 *            the message we want to check the content of
	 * @return true if our message's data is equal to EXIT
	 */
	private boolean foundExit(Message message) {
		// In this variable I am going to store the value of the message
		String valueOfMessage = message.getData();

		// If value of message is EXIT, we should return true
		if(valueOfMessage.equals(EXIT)) {
			return true;
		}
		// Else return false, as we did not found an EXIT command
		return false;
	}

	/**
	 * Helper function, returns true if we found END
	 *
	 * @param message
	 *            the message we want to check the content of
	 * @return true if our message's data is equal to END
	 */
	private boolean foundEnd(Message message) {
		// In this variable I am going to store the value of the message
		String valueOfMessage = message.getData();

		// If value of message is END, we should return true
		if(valueOfMessage.equals(END)) {
			return true;
		}
		// Else return false, as we did not found an END command
		return false;
	}
}
